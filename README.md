# BusyBoard

ESP32で実装されたビジーボードプログラム
ESP32 C++17で実装されたI2S / MPC23017 / 秋月7セグLEDシリアルドライバーモジュールなどが流用できそうなので公開しています。
よって、回路図などは掲載しておりません。

## コードについて

### I2Sサウンド関連
https://github.com/ayumu-bekki/busy_board/blob/develop/main/i2s_sound.h

MAX98357 I2S Class-D Mono Ampで動作確認しました。
https://learn.adafruit.com/adafruit-max98357-i2s-class-d-mono-amp

開始すると音声処理用のスレッドを作成し、任意のタイミングでデータを多重に再生する事ができます。

#### サウンドデータコンバート
以下のシェルスクリプトでresourceディレクトリ内にあるwavファイルをC++ヘッダに一括変換します。

```
resource/conv.sh
```

先ずsoxコマンドでWavのヘッダーを除外しRaw PCMデータとして保存しています。
これは、xxd -iオプションが標準入力では効かないための対応です。
その後、xxd -i でCヘッダファイル化し、sedでC++形式に整形しています。

### MCP23017  (IOエクスパンダ )
https://github.com/ayumu-bekki/busy_board/blob/develop/main/mcp23017.h

インプット周りは利用しなかったので今のところ未実装

### 赤色7セグメントLEDシリアルドライバーモジュール 
https://github.com/ayumu-bekki/busy_board/blob/develop/main/seg7_led.h

株式会社秋月電子通商の赤色7セグメントLEDシリアルドライバーモジュール(https://akizukidenshi.com/catalog/g/g118241/)を動作させるためのクラスです。
ただし、今回は1桁のみの利用だったため、複数桁表示については未実装
- 


