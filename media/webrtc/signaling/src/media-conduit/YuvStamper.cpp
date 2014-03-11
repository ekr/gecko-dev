/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nspr.h"
#include "YuvStamper.h"

namespace mozilla {

static unsigned char DIGIT_0 [] =
{
 0, 0, 1, 1, 0, 0,
 0, 1, 0, 0, 1, 0,
 1, 0, 0, 0, 0, 1,
 1, 0, 0, 0, 0, 1,
 1, 0, 0, 0, 0, 1,
 0, 1, 0, 0, 1, 0,
 0, 0, 1, 1, 0, 0
};

static unsigned char DIGIT_1 [] =
{
 0, 0, 0, 1, 0, 0,
 0, 0, 0, 1, 0, 0,
 0, 0, 0, 1, 0, 0,
 0, 0, 0, 1, 0, 0,
 0, 0, 0, 1, 0, 0,
 0, 0, 0, 1, 0, 0,
 0, 0, 0, 1, 0, 0,
};

static unsigned char DIGIT_2 [] =
{
 1, 1, 1, 1, 1, 0,
 0, 0, 0, 0, 0, 1,
 0, 0, 0, 0, 0, 1,
 0, 1, 1, 1, 1, 0,
 1, 0, 0, 0, 0, 0,
 1, 0, 0, 0, 0, 0,
 0, 1, 1, 1, 1, 1,
};

static unsigned char DIGIT_3 [] =
{
 1, 1, 1, 1, 1, 0,
 0, 0, 0, 0, 0, 1,
 0, 0, 0, 0, 0, 1,
 0, 1, 1, 1, 1, 1,
 0, 0, 0, 0, 0, 1,
 0, 0, 0, 0, 0, 1,
 1, 1, 1, 1, 1, 0,
};

static unsigned char DIGIT_4 [] =
{
 0, 1, 0, 0, 0, 1,
 0, 1, 0, 0, 0, 1,
 0, 1, 0, 0, 0, 1,
 0, 1, 1, 1, 1, 1,
 0, 0, 0, 0, 0, 1,
 0, 0, 0, 0, 0, 1,
 0, 0, 0, 0, 0, 1,
 0, 0, 0, 0, 0, 1
};

static unsigned char DIGIT_5 [] =
{
 0, 1, 1, 1, 1, 1,
 1, 0, 0, 0, 0, 0,
 1, 0, 0, 0, 0, 0,
 0, 1, 1, 1, 1, 0,
 0, 0, 0, 0, 0, 1,
 0, 0, 0, 0, 0, 1,
 1, 1, 1, 1, 1, 0,
};

static unsigned char DIGIT_6 [] =
{
 0, 1, 1, 1, 1, 1,
 1, 0, 0, 0, 0, 0,
 1, 0, 0, 0, 0, 0,
 1, 1, 1, 1, 1, 0,
 1, 0, 0, 0, 0, 1,
 1, 0, 0, 0, 0, 1,
 0, 1, 1, 1, 1, 0,
};

static unsigned char DIGIT_7 [] =
{
 1, 1, 1, 1, 1, 1,
 0, 0, 0, 0, 0, 1,
 0, 0, 0, 0, 0, 1,
 0, 0, 0, 0, 1, 0,
 0, 0, 0, 1, 0, 0,
 0, 0, 1, 0, 0, 0,
 0, 1, 0, 0, 0, 0,
 1, 0, 0, 0, 0, 0
};

static unsigned char DIGIT_8 [] =
{
 0, 1, 1, 1, 1, 1,
 1, 0, 0, 0, 0, 1,
 1, 0, 0, 0, 0, 1,
 0, 1, 1, 1, 1, 0,
 1, 0, 0, 0, 0, 1,
 1, 0, 0, 0, 0, 1,
 0, 1, 1, 1, 1, 0
};


static unsigned char DIGIT_9 [] =
{
 0, 1, 1, 1, 1, 1,
 1, 0, 0, 0, 0, 1,
 1, 0, 0, 0, 0, 1,
 0, 1, 1, 1, 1, 1,
 0, 0, 0, 0, 0, 1,
 0, 0, 0, 0, 0, 1,
 0, 1, 1, 1, 1, 0
};

static unsigned char *DIGITS[] = {
    DIGIT_0,
    DIGIT_1,
    DIGIT_2,
    DIGIT_3,
    DIGIT_4,
    DIGIT_5,
    DIGIT_6,
    DIGIT_7,
    DIGIT_8,
    DIGIT_9
};

bool YuvStamper::WritePixel(uint8_t *data, uint32_t x, uint32_t y) {
  if (x > width_)
    return false;

  if (y > height_)
    return false;

  uint8_t *ptr = &data[y * stride_ + x];
  *ptr = (*ptr > 96) ? 0 : 0xff;

  return true;
}

bool YuvStamper::WriteValue(uint8_t *data, uint32_t x, uint32_t y, uint8_t value) {
  if (x > width_)
    return false;

  if (y > height_)
    return false;

  uint8_t *ptr = &data[y * stride_ + x];
  *ptr = value;

  return true;
}

bool YuvStamper::ReadValue(uint8_t *data, uint32_t x, uint32_t y, uint8_t* value) {
  if (x > width_)
    return false;

  if (y > height_)
    return false;

  uint8_t *ptr = &data[y * stride_ + x];
  *value = *ptr;

  return true;
}


bool YuvStamper::WriteDigit(uint8_t* data, uint32_t x, uint32_t y, uint8_t digit) {
  if (digit > 9)
    return false;

  unsigned char *dig = DIGITS[digit];

  for (uint32_t row = 0; row < kDigitHeight; ++row) {
    for (uint32_t col = 0; col < kDigitWidth; ++col) {
      if (dig[(row * kDigitWidth) + col]) {
        for (uint32_t xx=0; xx < kPixelSize; ++xx) {
          for (uint32_t yy=0; yy < kPixelSize; ++yy) {
            if (!WritePixel(data, x + (col * kPixelSize) + xx, y + (row * kPixelSize) + yy))
              return false;
          }
        }
      }
    }
  }

  return true;
}

bool YuvStamper::Write(uint8_t *data, uint32_t value, uint32_t x, uint32_t y) {
  char buf[20];

  PR_snprintf(buf, sizeof(buf), "%.5u", value);

  for (size_t i=0; i<strlen(buf); ++i) {
    if (!WriteDigit(data, x, y, buf[i] - '0'))
      return false;

    x += kPixelSize * (kDigitWidth + 1);
  }

  return true;
}

// Encode a binary value as a big-endian binary value starting at x, y,
// with 1 being a 2x2 block of 0x80 pixels and 0 being a 2x2 block of
// 0 pixels.
bool YuvStamper::Encode(uint32_t width, uint32_t height, uint32_t stride,
                        uint8_t* data, const uint8_t* value, size_t len,
                        uint32_t x, uint32_t y) {
  YuvStamper stamper(width, height, stride);

  for (size_t i = 0; i < (len*8); ++i) {
    bool is_set = !!(value[i/8] & (0x80 >> i%8));
    for (uint32_t xx = 0; xx < kBitSize; ++xx) {
      for (uint32_t yy = 0; yy < kBitSize; ++yy) {
        if (!stamper.WriteValue(data, x + (i * kBitSize) + xx, y +yy,
                                is_set ? 0x80 : 0))
          return false;
      }
    }
  }
  return true;
}

// Decode a binary value as a big-endian binary value starting at x, y,
// with 1 being a 2x2 block of 0x80 pixels and 0 being a 2x2 block of
// 0 pixels.

bool YuvStamper::Decode(uint32_t width, uint32_t height, uint32_t stride,
                        uint8_t* data, uint8_t* value, size_t len,
                        uint32_t x, uint32_t y) {
  YuvStamper stamper(width, height, stride);

  memset(value, 0, len);

  for (size_t i = 0; i < (len*8); ++i) {
    uint32_t sum = 0;
    for (uint32_t xx = 0; xx < kBitSize; ++xx) {
      for (uint32_t yy = 0; yy < kBitSize; ++yy) {
        uint8_t val;
        if (!stamper.ReadValue(data, x + (i * kBitSize) + xx, y +yy, &val))
          return false;

        sum += val;
      }
    }
    if (sum > (kBitThreshold * kBitSize * kBitSize)) {
      value[i/8] |= (0x80 >> i%8);
    }
  }
  return true;
}


}  // Namespace mozilla.











