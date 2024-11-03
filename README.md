# BusyBoard

ESP32で実装されたビジーボードプログラム
ESP32 C++17で実装されたI2S / MPC23017 / 秋月7セグLEDシリアルドライバーモジュールなどが流用できそうなので後悔しています。
よって、回路図などは掲載しておりません。

- I2S
  - MAX98357 I2S Class-D Mono Ampで動作確認しました。
    - https://learn.adafruit.com/adafruit-max98357-i2s-class-d-mono-amp
- MCP23017  (IOエクスパンダ )
  - インプット周りは利用しなかったので未実装
- 赤色7セグメントLEDシリアルドライバーモジュール 
  - https://akizukidenshi.com/catalog/g/g118241/

