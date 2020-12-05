#include <Arduino.h>
#include <M5Stack.h>
#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <time.h>
#include <SPI.h>
#include <Ethernet.h>
//#include <BlynkSimpleEthernet.h>
#include <TimeLib.h>
#include <WidgetRTC.h> //RTCウィジェットを使う用
#include "Test/Test.h" //Testクラスを作ってみた。絶対パスを宣言しないと実行できなかった。

const char auth[] = "XXXXXXXXXXXXXXX"; //Blynkと連携するためのトークン宣言
const char ssid[] = "XXXXXXXXXXXXXXX"; //wifiネットワークのSSID宣言
const char pass[] = "XXXXXXXXXXXXXXX"; //wifiネットワークのパスワード宣言
const char address[] = "XXXXXXXXXXXXXX"; //通知のためのメールアドレス宣言

const char *fname = "/test.csv"; //test.csvを操作用ファイルとして宣言
File f; //File操作クラスのインスタンス
//String mojiretu; //csvデータ格納用
BlynkTimer timer; //virtualWrite使うなら必要
//int array[] = {}; //備忘録
WidgetRTC rtc; //RTCウィジェットのインスタンス
Test test; //今回は使わないが、一応クラスの関数が実行できた
bool isSosButtonPushed = false; //SOSボタンが押されたかどうか
bool isPamActive = true; //グローブが動いているかどうか判断
float activeTime = 0; //動作時間を格納する
int sensorParam = 1; //センサ感度を格納する

void finMail(float actTime)
{
  Blynk.email(address,"グローブ動作通知",String(actTime) + "秒作動しました。"); //グローブの動作終了時にメールを送る
}

//M5のABCボタンに対応させて実行する関数。テスト用
void buttonCheck()
{
  if (M5.BtnA.wasReleased())
  {
    M5.Lcd.println("A");
    isPamActive = true; //isPamActive
  }
  if (M5.BtnB.wasReleased())
  {
    M5.Lcd.println("B");
    isPamActive = false;
  }
  if (M5.BtnC.wasReleased())
  {
    M5.Lcd.println("C");
    finMail(activeTime);
  }
}

void ClockDisplay() //時刻計算用の関数。クラスに入れる予定
{
  String currentTime = String(hour()) + ":" + minute() + ":" + second();
  String currentDate = String(year()) + "/" + month() + "/" + day();
  M5.Lcd.print("Current time: ");
  M5.Lcd.println(currentTime); //現在の時刻を時間、分、秒の順にLCDに表示
  M5.Lcd.print("Current Date: ");
  M5.Lcd.println(currentDate); //現在の年、月、日の順にLCDに表示
}

//CSVファイルが開けるかどうか確かめる関数
void csvConfirm()
{
  f = SD.open(fname, FILE_READ); //test.csvのファイルを開く。FILE_READは読み込み専用モード
  //f = SD.open(fname,FILE_APPEND); //csvに書き込みを行う時にはこれを宣言。
  //同じ名前のファイルがないなら、新たにファイルを作成（何かしら書き込まないとファイルを作れない）

  if (!SD.begin())
  {
    M5.Lcd.println("ERROR:SD"); //SD.beginが作動したかどうか確かめる
  }
  if (f)
  {
    M5.Lcd.println("File open successful"); //test.csvが開かれていたらLcdに出力
    f.close();                              //csvファイルを閉じて保存する。書き込みが終わったら絶対閉じないといけない
  }
  else if (!f)
  {
    M5.Lcd.println("File Error"); //ファイルが見つからなければファイルエラーとして処理
  }
}


//準備用の関数。一度だけ実行される
void setup() 
{
  M5.begin();
  M5.Power.begin(); //M5の電源を入れる

  Serial.begin(115200); //シリアル通信レート115200
  delay(200);
  //SD.beginはM5.beginの中で処理されている
  M5.Lcd.setTextSize(2); //文字サイズを変更
  Blynk.begin(auth, ssid, pass); //この中にwifi.beginが入ってるからBlynk.begin(auth)とwifi.begin(ssid,pass)のようにわける
  setSyncInterval(10 * 60); //サーバ時刻との同期間隔を設定。デフォルトは300secらしい
  csvConfirm(); //csvの確認
}

//毎フレーム実行される関数
void loop() 
{
  Blynk.run(); //  Blynkの起動
  M5.update(); //ライブラリの変数を更新　M5のボタンを使うときには必ず必要になってくる
  delay(20);
  buttonCheck(); //ボタンが押された時実行する関数
}

//Blynkのサーバに接続された時に呼び出される関数
BLYNK_CONNECTED()
{
  Blynk.syncAll(); //今までの値の更新をBlynkにつないだ時にすべて受け取る
  rtc.begin();     //RTCを起動する。時刻を受け取るライブラリを使うために必要
}

//SOSボタンが押された時に呼ばれる////////////////////////
BLYNK_WRITE(V0)   //アプリ側からの値の更新があった場合に毎回呼び出される関数
{
  int v0Param = param[0].asInt(); //V0ピンの値をv0Paramに格納
  if (v0Param == HIGH) //ボタンが押された時実行
  {
    isSosButtonPushed = true;
  }
  if(isSosButtonPushed)
  {
    M5.Lcd.println("sosButton is Pressed"); //LCDに出力する
    Blynk.email(address,"Blynk","今、SOSボタンが押されました"); //メールでSOS通知
    ClockDisplay();//時刻をLCDに表示
    //この欄にスプレッドシート書き込みがほしい
    isSosButtonPushed = false;
  }
}

//センサ感度を管理する関数/////////////////////////
BLYNK_WRITE(V1)
{
  sensorParam = param[0].asInt(); //V1ピンの値をsensorParamに格納 
}

//センサ感度設定を確定する関数//////////////////////////
BLYNK_WRITE(V2)
{
  int setButtonValue = param[0].asInt(); //V2ピンの値をisSetButtonPushedに格納
  if(setButtonValue == HIGH)
  {
    if(!isPamActive)
    {
      M5.Lcd.print(sensorParam); //アプリ側で操作した値をLCDに表示
    }

  }
}

//ここからは検討中の関数///////////////////////////////////////////

/*
//setup loopよりも前に置かないとエラーがでそう
//csvの内容をString型に入れる関数。spreadsheetが使えるならいらないかも
void mojiretuPass()
{
  while(f.available()){
      mojiretu += char(f.read()); //読み取り可能な文字列がある限りchar型を文字列stringに追加
      //Blynk.email(adress,"ボタン押された",mojiretu);      
    }    
    //M5.Lcd.print(mojiretu);    
}
*/

/*
//CSV書き込み用の関数。spreadsheet使えるならいらないかも
void csvWrite()
{
  M5.Lcd.println("File open successful"); //test.csvが開かれていたらLcdに出力
  f.println(nowSec + "," + "second"); //test.csvに書き込む
  f.close(); //csvファイルを閉じて保存する
}
*/

