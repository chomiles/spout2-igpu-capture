## English

First public release of **Spout2 iGPU Capture**, an OBS source for receiving Warudo or another Spout2 sender from an integrated GPU and compositing on OBS's render GPU. Includes GPU auto-detection, manual selection, premultiplied transparency, and optional Intel E-core scheduling for capture.

- **Setup EXE:** Close OBS, run `Spout2-iGPU-Capture-0.1.0-Setup-x64.exe`, and reopen OBS.
- **Portable ZIP:** Close portable OBS and extract `Spout2-iGPU-Capture-0.1.0-Portable-x64.zip` into its root folder, beside `bin` and `data`.
- In OBS, add **Spout2 iGPU Capture** and select your **Spout Sender**. The source uses **Auto-detect Sender GPU** by default.

Tested on Windows 11 x64 with OBS Studio 33.0.3 and Intel Graphics → RTX 5070 Ti. AMD Radeon iGPU capture is unverified on hardware. The installer is unsigned. Use `SHA256SUMS.txt` to verify the downloads. [Full guide](https://github.com/chomiles/spout2-igpu-capture#english)

## 한국어

**Spout2 iGPU Capture**의 첫 공개 릴리스입니다. Warudo 등에서 내장 GPU로 보낸 Spout2 영상을 받아 OBS의 렌더링 GPU에서 합성합니다. GPU 자동 감지와 수동 선택, 사전 곱셈 알파 투명도, Intel E코어 수신 옵션을 지원합니다.

- **설치 EXE:** OBS를 닫고 `Spout2-iGPU-Capture-0.1.0-Setup-x64.exe`를 실행한 뒤 OBS를 다시 엽니다.
- **포터블 ZIP:** 포터블 OBS를 닫고 `Spout2-iGPU-Capture-0.1.0-Portable-x64.zip`을 `bin`과 `data`가 있는 OBS 최상위 폴더에 풉니다.
- OBS에 **Spout2 iGPU Capture** 소스를 추가한 뒤 **Spout Sender**를 고릅니다. 기본값은 **Auto-detect Sender GPU**입니다.

Windows 11 x64의 OBS Studio 33.0.3에서 Intel Graphics → RTX 5070 Ti 구성을 검증했습니다. AMD Radeon iGPU 수신은 실기기에서 아직 검증하지 않았습니다. 설치 EXE는 서명되지 않았으며 `SHA256SUMS.txt`에서 다운로드 해시를 확인할 수 있습니다. [전체 사용법](https://github.com/chomiles/spout2-igpu-capture#한국어)

## 日本語

**Spout2 iGPU Capture** の初回公開リリースです。Warudo などが内蔵 GPU で送信した Spout2 映像を受け取り、OBS の描画用 GPU で合成します。GPU の自動検出と手動選択、乗算済みアルファの透過、Intel E コアをキャプチャに使うオプションを備えています。

- **インストーラー EXE:** OBS を終了し、`Spout2-iGPU-Capture-0.1.0-Setup-x64.exe` を実行してから OBS を起動します。
- **ポータブル ZIP:** ポータブル版 OBS を終了し、`Spout2-iGPU-Capture-0.1.0-Portable-x64.zip` を `bin` と `data` がある OBS のルートフォルダーに展開します。
- OBS に **Spout2 iGPU Capture** を追加し、**Spout Sender** を選択します。既定値は **Auto-detect Sender GPU** です。

Windows 11 x64 の OBS Studio 33.0.3 で、Intel Graphics → RTX 5070 Ti の構成を確認しました。AMD Radeon iGPU での受信は実機で未検証です。インストーラーは未署名です。ダウンロードの確認には `SHA256SUMS.txt` を使用できます。 [詳しい使い方](https://github.com/chomiles/spout2-igpu-capture#日本語)
