// Copyright 2019 David Conran
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ir_Argo.h"
#include "IRac.h"
#include "IRrecv.h"
#include "IRrecv_test.h"
#include "IRsend.h"
#include "IRsend_test.h"


TEST(TestArgoACClass, toCommon) {
  IRArgoAC ac(kGpioUnused);
  ac.setPower(true);
  ac.setMode(kArgoCool);
  ac.setTemp(20);
  ac.setFan(kArgoFan3);
  ac.setMax(true);
  ac.setNight(true);
  // Now test it.
  ASSERT_EQ(decode_type_t::ARGO, ac.toCommon().protocol);
  ASSERT_TRUE(ac.toCommon().power);
  ASSERT_TRUE(ac.toCommon().celsius);
  ASSERT_EQ(20, ac.toCommon().degrees);
  ASSERT_EQ(stdAc::opmode_t::kCool, ac.toCommon().mode);
  ASSERT_EQ(stdAc::fanspeed_t::kMax, ac.toCommon().fanspeed);
  ASSERT_EQ(0, ac.toCommon().sleep);
  ASSERT_TRUE(ac.toCommon().turbo);
  // Unsupported.
  ASSERT_EQ(-1, ac.toCommon().model);
  ASSERT_EQ(stdAc::swingv_t::kOff, ac.toCommon().swingv);
  ASSERT_EQ(stdAc::swingh_t::kOff, ac.toCommon().swingh);
  ASSERT_FALSE(ac.toCommon().econo);
  ASSERT_FALSE(ac.toCommon().light);
  ASSERT_FALSE(ac.toCommon().filter);
  ASSERT_FALSE(ac.toCommon().clean);
  ASSERT_FALSE(ac.toCommon().beep);
  ASSERT_FALSE(ac.toCommon().quiet);
  ASSERT_EQ(-1, ac.toCommon().clock);
}

TEST(TestArgoACClass, MessageConstructon) {
  IRArgoAC ac(kGpioUnused);
  ac.setPower(true);
  ac.setTemp(20);
  ac.setMode(kArgoCool);
  ac.setFan(kArgoFanAuto);
  ac.setRoomTemp(21);
  ac.setiFeel(true);
  ac.setMax(true);
  ac.setNight(true);

  // Don't implicitly trust this. It's just a guess.
  auto expected = std::vector<uint8_t>({
      0xAC, 0xF5, 0x00, 0x24, 0x02, 0x00, 0x00, 0x00, 0x00, 0xAC, 0xD6, 0x01});
  auto actual = ac.getRaw();
  EXPECT_THAT(std::vector<uint8_t>(actual, actual + kArgoBits / 8),
              ::testing::ElementsAreArray(expected));
  EXPECT_EQ(
      "Power: On, Mode: 0 (Cool), Fan: 0 (Auto), Temp: 20C, Room Temp: 21C, "
      "Max: On, IFeel: On, Night: On",
      ac.toString());
}

// Tests for sendArgo().

// Test sending typical data only.
TEST(TestSendArgo, SendDataOnly) {
  IRsendTest irsend(0);
  irsend.begin();
  uint8_t data[kArgoStateLength] = {
      0xAC, 0xF5, 0x00, 0x24, 0x02, 0x00, 0x00, 0x00, 0x00, 0xAC, 0xD6, 0x01};

  irsend.sendArgo(data);
  EXPECT_EQ(
      "f38000d50"
      "m6400s3300"
      "m400s900m400s900m400s2200m400s2200m400s900m400s2200m400s900m400s2200"
      "m400s2200m400s900m400s2200m400s900m400s2200m400s2200m400s2200m400s2200"
      "m400s900m400s900m400s900m400s900m400s900m400s900m400s900m400s900"
      "m400s900m400s900m400s2200m400s900m400s900m400s2200m400s900m400s900"
      "m400s900m400s2200m400s900m400s900m400s900m400s900m400s900m400s900"
      "m400s900m400s900m400s900m400s900m400s900m400s900m400s900m400s900"
      "m400s900m400s900m400s900m400s900m400s900m400s900m400s900m400s900"
      "m400s900m400s900m400s900m400s900m400s900m400s900m400s900m400s900"
      "m400s900m400s900m400s900m400s900m400s900m400s900m400s900m400s900"
      "m400s900m400s900m400s2200m400s2200m400s900m400s2200m400s900m400s2200"
      "m400s900m400s2200m400s2200m400s900m400s2200m400s900m400s2200m400s2200"
      "m400s2200m400s900m400s900m400s900m400s900m400s900m400s900"
      "m400s900",
      irsend.outputStr());
}

// Tests for decodeArgo().
// Decode normal Argo messages.

TEST(TestDecodeArgo, SyntheticDecode) {
  IRsendTest irsend(kGpioUnused);
  IRrecv irrecv(kGpioUnused);
  irsend.begin();

  // Synthesised Normal Argo message.
  irsend.reset();
  uint8_t expectedState[kArgoStateLength] = {
      0xAC, 0xF5, 0x00, 0x24, 0x02, 0x00, 0x00, 0x00, 0x00, 0xAC, 0xD6, 0x01};
  irsend.sendArgo(expectedState);
  irsend.makeDecodeResult();
  EXPECT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(decode_type_t::ARGO, irsend.capture.decode_type);
  EXPECT_EQ(kArgoBits, irsend.capture.bits);
  EXPECT_STATE_EQ(expectedState, irsend.capture.state, irsend.capture.bits);
  EXPECT_EQ(
      "Power: On, Mode: 0 (Cool), Fan: 0 (Auto), Temp: 20C, Room Temp: 21C, "
      "Max: On, IFeel: On, Night: On",
      IRAcUtils::resultAcToString(&irsend.capture));
  stdAc::state_t r, p;
  ASSERT_TRUE(IRAcUtils::decodeToState(&irsend.capture, &r, &p));
}

TEST(TestArgoACClass, SetAndGetTemp) {
  IRArgoAC ac(kGpioUnused);

  ac.setTemp(25);
  EXPECT_EQ(25, ac.getTemp());
  ac.setTemp(kArgoMinTemp);
  EXPECT_EQ(kArgoMinTemp, ac.getTemp());
  ac.setTemp(kArgoMaxTemp);
  EXPECT_EQ(kArgoMaxTemp, ac.getTemp());
  ac.setTemp(kArgoMinTemp - 1);
  EXPECT_EQ(kArgoMinTemp, ac.getTemp());
  ac.setTemp(kArgoMaxTemp + 1);
  EXPECT_EQ(kArgoMaxTemp, ac.getTemp());
}

TEST(TestArgoACClass, SetAndGetRoomTemp) {
  IRArgoAC ac(kGpioUnused);

  ac.setRoomTemp(25);
  EXPECT_EQ(25, ac.getRoomTemp());
  ac.setRoomTemp(kArgoTempDelta);
  EXPECT_EQ(kArgoTempDelta, ac.getRoomTemp());
  ac.setRoomTemp(kArgoMaxRoomTemp);
  EXPECT_EQ(kArgoMaxRoomTemp, ac.getRoomTemp());
  ac.setRoomTemp(kArgoTempDelta - 1);
  EXPECT_EQ(kArgoTempDelta, ac.getRoomTemp());
  ac.setRoomTemp(kArgoMaxRoomTemp + 1);
  EXPECT_EQ(kArgoMaxRoomTemp, ac.getRoomTemp());
}

TEST(TestArgoACClass, SetAndGetMode) {
  IRArgoAC ac(kGpioUnused);

  ac.setMode(kArgoHeat);
  EXPECT_EQ(kArgoHeat, ac.getMode());
  ac.setMode(kArgoCool);
  EXPECT_EQ(kArgoCool, ac.getMode());
  ac.setMode(kArgoDry);
  EXPECT_EQ(kArgoDry, ac.getMode());
  ac.setMode(kArgoAuto);
  EXPECT_EQ(kArgoAuto, ac.getMode());
  ac.setMode(kArgoHeatAuto);
  EXPECT_EQ(kArgoHeatAuto, ac.getMode());
  ac.setMode(kArgoOff);
  EXPECT_EQ(kArgoOff, ac.getMode());
  ac.setMode(255);
  EXPECT_EQ(kArgoAuto, ac.getMode());
}

TEST(TestArgoACClass, SetAndGetFan) {
  IRArgoAC ac(kGpioUnused);

  ac.setFan(kArgoFan3);
  EXPECT_EQ(kArgoFan3, ac.getFan());
  ac.setFan(kArgoFan1);
  EXPECT_EQ(kArgoFan1, ac.getFan());
  ac.setFan(kArgoFanAuto);
  EXPECT_EQ(kArgoFanAuto, ac.getFan());
  ac.setFan(kArgoFan3);
  EXPECT_EQ(kArgoFan3, ac.getFan());
  ASSERT_NE(7, kArgoFan3);
  // Now try some unexpected value.
  ac.setFan(7);
  EXPECT_EQ(kArgoFan3, ac.getFan());
}

TEST(TestArgoACClass, Night) {
  IRArgoAC ac(kGpioUnused);
  ac.setNight(false);
  ASSERT_FALSE(ac.getNight());
  ac.setNight(true);
  ASSERT_TRUE(ac.getNight());
  ac.setNight(false);
  ASSERT_FALSE(ac.getNight());
}

TEST(TestArgoACClass, iFeel) {
  IRArgoAC ac(kGpioUnused);
  ac.setiFeel(false);
  ASSERT_FALSE(ac.getiFeel());
  ac.setiFeel(true);
  ASSERT_TRUE(ac.getiFeel());
  ac.setiFeel(false);
  ASSERT_FALSE(ac.getiFeel());
}

TEST(TestArgoACClass, Power) {
  IRArgoAC ac(kGpioUnused);
  ac.setPower(false);
  ASSERT_FALSE(ac.getPower());
  ac.setPower(true);
  ASSERT_TRUE(ac.getPower());
  ac.setPower(false);
  ASSERT_FALSE(ac.getPower());
}

TEST(TestArgoACClass, Max) {
  IRArgoAC ac(kGpioUnused);
  ac.setMax(false);
  ASSERT_FALSE(ac.getMax());
  ac.setMax(true);
  ASSERT_TRUE(ac.getMax());
  ac.setMax(false);
  ASSERT_FALSE(ac.getMax());
}

TEST(TestUtils, Housekeeping) {
  ASSERT_EQ("ARGO", typeToString(decode_type_t::ARGO));
  ASSERT_EQ(decode_type_t::ARGO, strToDecodeType("ARGO"));
  ASSERT_TRUE(hasACState(decode_type_t::ARGO));
}

TEST(TestDecodeArgo, RealShortDecode) {
  IRsendTest irsend(kGpioUnused);
  IRrecv irrecv(kGpioUnused);
  irsend.begin();

  // Real short Argo message. (i.e. 32 bits)
  const uint16_t sensor_28C[67] = {
      6418, 3168, 444, 834, 444, 834, 442, 2112, 444, 2112, 444, 834, 442, 2114,
      442, 834, 442, 2112, 444, 2112, 444, 832, 442, 2114, 442, 834, 442, 2112,
      442, 2114, 442, 2112, 444, 2112, 442, 834, 442, 2112, 444, 834, 442, 834,
      442, 834, 442, 834, 442, 2114, 442, 2114, 442, 2112, 442, 2112, 442, 834,
      444, 834, 442, 834, 442, 2112, 442, 2112, 442, 836,
      442};  // UNKNOWN 6149090
  irsend.reset();
  uint8_t expectedState[kArgoShortStateLength] = {
      0xAC, 0xF5, 0xC2, 0x63};
  irsend.sendRaw(sensor_28C, 67, 38);
  irsend.makeDecodeResult();
  EXPECT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(decode_type_t::ARGO, irsend.capture.decode_type);
  EXPECT_EQ(kArgoShortBits, irsend.capture.bits);
  EXPECT_STATE_EQ(expectedState, irsend.capture.state, irsend.capture.bits);
  EXPECT_EQ(
      "Sensor Temp: 28C",
      IRAcUtils::resultAcToString(&irsend.capture));
  stdAc::state_t r, p;
  // These short messages don't result in a valid state.
  ASSERT_FALSE(IRAcUtils::decodeToState(&irsend.capture, &r, &p));
}
