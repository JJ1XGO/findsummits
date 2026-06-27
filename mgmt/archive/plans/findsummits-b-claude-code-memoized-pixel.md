# 計画: `-b` で Claude Code を毎回必ず最新化する

## Context（背景）

`./claude-container -b` は `podman compose run --build` を実行するだけで、`--no-cache` ではない。
Claude Code をインストールする行（`Dockerfile.claude:56`）は命令文字列が常に同じ（`bash -s latest`）ため、Podman はキャッシュ済みレイヤーを使い回し、イメージに焼き込まれた Claude Code のバージョンが**凍結**される。
さらに `DISABLE_AUTOUPDATER: "1"`（`compose.yml:17`）で実行時の自動更新も無効化済みのため、放置すると古いバージョンに固定され続ける。

**目的**: `-b` を付けたビルドでは、install 行から下のキャッシュだけをピンポイントで破棄し、`install.sh` を必ず再実行して最新版を取得する。apt パッケージ等の重い上位レイヤーはキャッシュを流用して速度を保つ。

## 仕組み（cache-busting）

ビルドのたびに値が変わる ARG（`CACHEBUST`）を install 行の直前に置き、install 行の RUN 内で参照する。
値が毎回変わる → Podman が「ARG の初回使用箇所でキャッシュミス」と判定 → install 行以降だけ再実行。
値の受け渡しは既存の `TZ` と同じく compose.yml の `build.args` 経由（環境変数補間）に統一する。

## 変更内容

### 1. `Dockerfile.claude`
install 行（56行目付近）を以下に変更。`ARG` を定義し、RUN 内で `${CACHEBUST}` を**参照**する（参照しないとキャッシュ破棄されない）。

```dockerfile
# Cache-bust marker: changing this ARG forces the install layer (and below)
# to rebuild, so `-b` always fetches the latest Claude Code.
ARG CACHEBUST=unknown

# Install Claude Code via official native installer (Recommended method)
RUN echo "cachebust=${CACHEBUST}" \
  && curl -fsSL https://claude.ai/install.sh | bash -s ${CLAUDE_CODE_VERSION}
```

### 2. `compose.yml`
`build.args` に `CACHEBUST` を追加（TZ と同じ補間パターン）。

```yaml
    build:
      context: ${CLAUDE_CONTAINER_DIR}
      dockerfile: ${CLAUDE_CONTAINER_DIR}/Dockerfile.claude
      args:
        TZ: ${TZ:-UTC}
        CACHEBUST: ${CACHEBUST:-unknown}
```

### 3. `claude-container`
`-b` 分岐（89行目）でのみ `CACHEBUST` に現在時刻（エポック秒）を設定して渡す。
`-b` なし分岐はビルド自体走らないため変更不要。

```bash
if [[ $BUILD -eq 1 ]]; then
  CACHEBUST="$(date +%s)" CONTEXT="$WORKING_DIR" CLAUDE_CONTAINER_DIR="$RUN_DIR" \
    podman compose -f "$RUN_DIR/compose.yml" --in-pod false run --rm --build claude-auth-workspace
else
  CONTEXT="$WORKING_DIR" CLAUDE_CONTAINER_DIR="$RUN_DIR" \
    podman compose -f "$RUN_DIR/compose.yml" --in-pod false run --rm claude-auth-workspace
fi
```

### 4. `README.md`（日英両方）
現状の記述「リビルドするとその時点の最新版がインストールされる」は、キャッシュにより**実際には凍結される**ため不正確。今回の修正で初めて正しくなるので、仕組みに触れる形へ更新する。

- **日本語 L70**:「`CLAUDE_CODE_VERSION` ビルド引数はデフォルト `latest` のため…」→ `-b` 時に install レイヤーのキャッシュを破棄して必ず最新版を取得する旨を追記。
- **英語 L177**: 同内容を英語で更新（`-b` busts the install-layer cache via a `CACHEBUST` build arg so the latest Claude Code is always fetched）。
- L72 / L179 の自動アップデート無効化の説明はそのまま整合するので原則維持（必要なら文言微調整）。

### 5. `CLAUDE.md`（プロジェクト）
- 「Modifying the Image」節: `CACHEBUST` による `-b` の挙動を追記。
- 「Architecture」のファイル説明: `claude-container`（`-b` 時に `CACHEBUST` を渡す）/ `compose.yml`（`build.args` に `CACHEBUST`）/ `Dockerfile.claude`（install 行直前の `ARG CACHEBUST`）に一文ずつ追記。

> ドキュメントは「勝手な変更禁止」規約があるが、本件は挙動変更に伴う**事実の同期**であり、ユーザー承認済みとして扱う。

## 検証

1. 構文チェック: `bash -n claude-container`
2. Compose 検証: `CACHEBUST=test podman compose -f compose.yml config`（args に CACHEBUST が出ることを確認）
3. 実動作: `./claude-container -b <dir>` を実行し、ビルドログで install.sh の RUN がキャッシュを使わず再実行されていることを確認（`--> Using cache` が install 行に出ない）。
4. 連続実行: 直後にもう一度 `./claude-container -b <dir>` し、install 行が再び再実行される（毎回最新化される）ことを確認。apt 等の上位レイヤーは `Using cache` のままで速いことも確認。
5. 起動後コンテナ内で `claude --version` が最新であることを確認。

## 補足

- 実装後、本計画ファイルを `/workspace/.claude/mgmt/plan.md` へ移動する（プロジェクト規約）。
- コミットは Conventional Commits 形式・日本語本文で、一連の作業完了後にまとめて1回（push は別途指示）。
