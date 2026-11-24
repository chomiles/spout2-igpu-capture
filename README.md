# Spout2 iGPU Capture

[English](#english) · [한국어](#한국어) · [日本語](#日本語)

## English

Spout2 iGPU Capture is a Windows OBS Studio source for setups where an app such as Warudo renders on an integrated GPU while OBS renders on a discrete GPU. It receives the Spout2 texture on the sender's GPU and copies the frame into OBS for compositing. It carries uncompressed video and preserves premultiplied transparency from 8-bit RGBA/BGRA sources.

Use it when you want to keep the avatar renderer on a separate GPU and avoid an intermediate video codec. Frame transfers between the GPUs still use system memory. [Downloads and source](https://github.com/chomiles/spout2-igpu-capture/releases)

### Install

**Normal OBS installation (EXE)**

1. Close OBS Studio.
2. Run `Spout2-iGPU-Capture-<version>-Setup-x64.exe` and follow the wizard. It installs to `C:\ProgramData\obs-studio\plugins\warudo-spout-bridge\`.
3. Start OBS. To update, run the new installer with OBS closed. To remove it, use **Windows Settings → Apps → Installed apps → Spout2 iGPU Capture**.

**Portable OBS (ZIP)**

1. Close the portable OBS instance. Extract `Spout2-iGPU-Capture-<version>-Portable-x64.zip` into the **portable OBS root**, the folder containing `bin` and `data`. The ZIP places the DLL at `plugins\warudo-spout-bridge\warudo-spout-bridge.dll`.
2. Start OBS in portable mode as usual. To update, extract the latest ZIP into the same location with OBS closed. The same ZIP can be used for a manual install by copying its `warudo-spout-bridge` folder to `C:\ProgramData\obs-studio\plugins\`.

### Set up the source

1. In Warudo, open **Camera → Output → Spout Output** and turn it on. Turn on **Transparent Background** if you need transparency.
2. In OBS, add **Spout2 iGPU Capture** under **Sources**. Choose the **Spout Sender**, such as `Warudo Camera 1`.
3. Leave **Spout Sender GPU** on **Auto-detect Sender GPU**, or select the GPU actually used by Warudo. **OBS Render GPU** shows the adapter OBS is using for compositing.
4. Optionally enable **Use Intel E-cores for Capture** on a supported Intel hybrid CPU. This setting applies to the plugin's capture thread. For a Spout-only workflow, turn off any unused NDI output in Warudo.

The initial release was tested on Windows 11 x64 with **OBS Studio 33.0.3**, Intel Graphics as the sender, and an RTX 5070 Ti as the OBS render GPU. GPU selection uses the Windows adapter list. Capture on AMD Radeon iGPUs has yet to be tested on hardware. One 60fps Warudo comparison on the reference PC showed motion arriving about 33–50 ms earlier than its local NDI input. Results depend on the sender and your system.

The installer is unsigned. The release includes SHA-256 checksums for both downloads. [Build instructions and third-party notices](https://github.com/chomiles/spout2-igpu-capture/blob/main/BUILDING.md)

## 한국어

Spout2 iGPU Capture는 Warudo 같은 앱을 내장 GPU에서 렌더링하고 OBS를 외장 GPU에서 실행할 때 사용하는 Windows용 OBS 소스입니다. 송신 GPU에서 Spout2 텍스처를 받아 OBS가 합성할 수 있도록 프레임을 복사합니다. 8비트 RGBA/BGRA 영상을 압축 없이 전달하고 사전 곱셈 알파 투명도를 유지합니다.

아바타 렌더링을 별도 GPU에 두면서 중간 영상 코덱을 생략하고 싶을 때 사용합니다. GPU 사이의 프레임 전송에는 시스템 메모리를 사용합니다. [다운로드와 소스 코드](https://github.com/chomiles/spout2-igpu-capture/releases)

### 설치

**일반 설치형 OBS (EXE)**

1. OBS Studio를 종료합니다.
2. `Spout2-iGPU-Capture-<version>-Setup-x64.exe`를 실행하고 설치 마법사를 따릅니다. 설치 경로는 `C:\ProgramData\obs-studio\plugins\warudo-spout-bridge\`입니다.
3. OBS를 실행합니다. 업데이트할 때는 OBS를 닫고 새 설치 파일을 실행합니다. 삭제는 **Windows 설정 → 앱 → 설치된 앱 → Spout2 iGPU Capture**에서 할 수 있습니다.

**포터블 OBS (ZIP)**

1. 포터블 OBS를 종료합니다. `Spout2-iGPU-Capture-<version>-Portable-x64.zip`을 `bin`과 `data`가 있는 **포터블 OBS 최상위 폴더**에 풉니다. DLL 경로는 `plugins\warudo-spout-bridge\warudo-spout-bridge.dll`입니다.
2. 평소처럼 OBS를 포터블 모드로 실행합니다. 업데이트할 때는 OBS를 닫고 같은 경로에 새 ZIP을 풉니다. 일반 설치형 OBS에 수동 설치하려면 ZIP 안의 `warudo-spout-bridge` 폴더를 `C:\ProgramData\obs-studio\plugins\`에 복사합니다.

### 소스 설정

1. Warudo에서 **Camera → Output → Spout Output**을 켭니다. 투명 배경이 필요하면 **Transparent Background**를 켭니다.
2. OBS의 **소스 목록**에서 **Spout2 iGPU Capture**를 추가하고 **Spout Sender**에 `Warudo Camera 1` 같은 송신자를 선택합니다.
3. **Spout Sender GPU**는 **Auto-detect Sender GPU**로 두거나 Warudo가 실제 사용하는 GPU를 지정합니다. **OBS Render GPU**에는 OBS가 합성에 사용하는 GPU가 표시됩니다.
4. 지원하는 Intel 하이브리드 CPU에서는 **Use Intel E-cores for Capture**를 선택할 수 있습니다. 이 옵션은 플러그인의 수신 스레드에 적용됩니다. Spout만 사용할 때는 Warudo에서 쓰지 않는 NDI 출력을 끕니다.

첫 릴리스는 Windows 11 x64의 **OBS Studio 33.0.3**에서 Intel Graphics로 송신하고 RTX 5070 Ti로 합성해 검증했습니다. GPU 선택 목록은 Windows가 인식한 어댑터를 사용합니다. AMD Radeon iGPU에서의 영상 수신은 실기기 검증이 필요합니다. 검증 PC의 60fps Warudo 비교에서는 로컬 NDI보다 움직임이 약 33~50ms 먼저 표시됐습니다. 결과는 송신 앱과 PC 구성에 따라 달라집니다.

설치 실행 파일은 서명되지 않았습니다. 두 다운로드의 SHA-256 체크섬은 릴리스에 포함되어 있습니다. [빌드 방법과 외부 코드 고지](https://github.com/chomiles/spout2-igpu-capture/blob/main/BUILDING.md)

## 日本語

Spout2 iGPU Capture は、Warudo などのアプリを内蔵 GPU で描画し、OBS をディスクリート GPU で動かす構成向けの Windows 用 OBS ソースです。送信側 GPU で Spout2 テクスチャを受け取り、OBS の合成用にフレームをコピーします。8 ビット RGBA/BGRA の映像を非圧縮で渡し、乗算済みアルファの透明度を保持します。

アバターの描画を別の GPU に任せたまま、中間の映像コーデック処理を省きたい場合に使います。GPU 間のフレーム転送にはシステムメモリを使用します。 [ダウンロードとソースコード](https://github.com/chomiles/spout2-igpu-capture/releases)

### インストール

**通常インストール版の OBS (EXE)**

1. OBS Studio を終了します。
2. `Spout2-iGPU-Capture-<version>-Setup-x64.exe` を実行し、ウィザードに従います。インストール先は `C:\ProgramData\obs-studio\plugins\warudo-spout-bridge\` です。
3. OBS を起動します。更新するときは OBS を終了してから新しいインストーラーを実行します。削除は **Windows の設定 → アプリ → インストールされているアプリ → Spout2 iGPU Capture** から行えます。

**ポータブル版の OBS (ZIP)**

1. ポータブル版の OBS を終了します。`Spout2-iGPU-Capture-<version>-Portable-x64.zip` を、`bin` と `data` がある **ポータブル版 OBS のルートフォルダー**に展開します。DLL は `plugins\warudo-spout-bridge\warudo-spout-bridge.dll` に配置されます。
2. 普段どおり OBS をポータブルモードで起動します。更新するときは OBS を終了し、同じ場所に新しい ZIP を展開します。通常インストール版に手動で追加する場合は、ZIP 内の `warudo-spout-bridge` フォルダーを `C:\ProgramData\obs-studio\plugins\` にコピーします。

### ソースの設定

1. Warudo の **Camera → Output → Spout Output** をオンにします。背景を透過させる場合は **Transparent Background** もオンにします。
2. OBS の**ソース**に **Spout2 iGPU Capture** を追加し、**Spout Sender** で `Warudo Camera 1` などの送信元を選びます。
3. **Spout Sender GPU** は **Auto-detect Sender GPU** のまま使うか、Warudo が実際に使っている GPU を指定します。**OBS Render GPU** には OBS が合成に使う GPU が表示されます。
4. 対応する Intel ハイブリッド CPU では **Use Intel E-cores for Capture** をオンにできます。この設定はプラグインの受信スレッドに適用されます。Spout のみを使う場合は、Warudo 側で使わない NDI 出力をオフにします。

初回リリースは Windows 11 x64 の **OBS Studio 33.0.3**、送信側が Intel Graphics、OBS 側が RTX 5070 Ti の構成で確認しています。GPU 選択には Windows が認識したアダプターが表示されます。AMD Radeon iGPU での映像受信は実機で未検証です。検証 PC の 60fps の Warudo では、同一 PC の NDI 入力より動きが約 33～50 ms 早く表示されました。結果は送信アプリや PC 構成によって異なります。

インストーラーにはコード署名がありません。両方のダウンロードの SHA-256 チェックサムをリリースに掲載しています。 [ビルド手順とサードパーティーコードの情報](https://github.com/chomiles/spout2-igpu-capture/blob/main/BUILDING.md)
