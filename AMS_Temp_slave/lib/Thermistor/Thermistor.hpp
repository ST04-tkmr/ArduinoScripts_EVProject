/*
 *  電圧の測定はサーミスタの電圧降下を測定
 *
 *      .+5V
 *      |
 *      _
 *     | | R=10kΩ
 *     | |
 *      -
 *      |
 *      .-----.Vd(アナログピンで読み取り、10bitADCで変換されるので実際の電圧にするには4.9mVを掛け算する)
 *      |
 *      _
 *     | | サーミスタ r
 *     | |
 *      -
 *      |
 *      .GND
 *
 *    Vd = { 5/(10k+r) } x r よりrを算出
 *
 *  温度T0=25度のときのサーミスタの抵抗値R0=10kΩ
 *  データシート(https://www.murata.com/ja-jp/products/productdetail?partno=NXFT15XH103FEAB021)よりB定数(25度/50度)は3380K
 *  データシートがこれであってるかはわからない
 *
 *  抵抗から温度を算出する式
 *    T = 1/{ln(r/R0)/B + 1/(T0+273)} -273
 */

#ifndef AMS_TEMP_MONITOR
#define AMS_TEMP_MONITOR

#include <math.h>

//-------------------------------------------------------
//  型定義
//-------------------------------------------------------
//サーミスタのパラメータ
//アナログピンで読み取った値と算出したサーミスタの情報を格納する
class Thermistor {
    private:
    volatile int val;   //アナログピンで読み取った値
    volatile float r;   //抵抗値
    volatile float temp;    //抵抗値から計算した温度

    public:
    Thermistor();

    //-------------------------------------------------------
    //  アナログピンで読み取った値をセットする
    //  セットした値から電圧値に換算し、抵抗値と温度も計算してセットする
    //  引数：アナログピンで読み取った値(analogReadの戻り値0~1023)
    //  戻り値：成功1, 失敗0
    //-------------------------------------------------------
    char setVal(int val);

    int getVal();
    float getR();
    float getTemp();

    //-------------------------------------------------------
    //  読み込んだ電圧からサーミスタの抵抗を計算
    //  引数：アナログピンで読み取った値(analogReadの戻り値0~1023)
    //  戻り値：サーミスタの抵抗値
    //-------------------------------------------------------
    float calcR(int val);

    //-------------------------------------------------------
    //  抵抗から温度計算
    //  引数：サーミスタの抵抗値
    //  戻り値：サーミスタの温度
    //-------------------------------------------------------
    float calcTemp(float r);
};

inline int Thermistor::getVal() {
    return val;
}

inline float Thermistor::getR() {
    return r;
}

inline float Thermistor::getTemp() {
    return temp;
}

#endif