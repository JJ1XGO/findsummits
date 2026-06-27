# コンテナ側 requirements.txt を役割明記コメントのみにする

## Context
findsummits（devel）の Python 依存は、findsummits 側の `requirements.txt`
（requests/openpyxl/shapely/numpy/pillow）を `make venv-rebuild` で
プロジェクトルートの `venv/` に隔離構築している（venv 内で完結）。

一方このコンテナの `requirements.txt`（numpy/requests）は
`pip3 install --break-system-packages` でシステム Python に入るだけで、
findsummits の venv とは独立。findsummits の環境構築には使われておらず、
役割が曖昧。findsummits の venv 構築に必要なのは `packages.txt` 側の
`make`/`python3-venv`/`python3-pip` であり、こちらの requirements.txt の
パッケージは不要。

→ コンテナ側 `requirements.txt` を空（役割明記コメントのみ）にする。

## 変更内容
- `requirements.txt`：numpy/requests を削除し、役割を説明する日本語コメントのみにする
  - findsummits の Python 依存は findsummits 側 venv で管理する旨を明記
  - 将来 venv 外（システム Python）で必要なパッケージが出た場合の追記場所である旨

## 補足（挙動上の確認）
- `Dockerfile.claude:34` の `pip3 install -r /tmp/requirements.txt --break-system-packages`
  は、コメントのみ（パッケージ0件）のファイルでも正常終了する（エラーにならない）。
  Dockerfile 側の変更は不要。

## 検証
- `bash -n claude-container`（スクリプトは変更しないが念のため）
- `podman compose -f compose.yml config` で Compose 妥当性確認
- 実ビルド確認（任意）：`./claude-container -b <findsummits dir>` で
  pip install 層がエラーなく通ることを確認
