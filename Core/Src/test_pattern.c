#include "test_pattern.h"
#include "stm32f7xx_hal.h"
/*----------------------------------------------------------*/
/*----------------------------------------------------------*/
/*----------------------------------------------------------*/

// === synthetic pattern metadata ===
static TestPatternInfo_t g_pattern_info;

/*----------------------------------------------------------*/
/*----------------------------------------------------------*/
/*----------------------------------------------------------*/

static void TestPattern_FillRealisticBody(uint8_t bed_value[BED_ROWS][BED_COLS],
                                          uint8_t bed_status[BED_ROWS][BED_COLS],
                                          uint8_t bed_valid[BED_ROWS][BED_COLS],
                                          uint8_t center,
                                          int sacrum_boost,
                                          int heel_left_boost,
                                          int heel_right_boost,
                                          int body_scale_bias);


static void TestPattern_FillTurningCycleSmooth(uint8_t bed_value[BED_ROWS][BED_COLS],
                                               uint8_t bed_status[BED_ROWS][BED_COLS],
                                               uint8_t bed_valid[BED_ROWS][BED_COLS]);
/*----------------------------------------------------------*/

// === helper: fill whole bed with default "no body contact" values ===
static void TestPattern_ClearBed(uint8_t bed_value[BED_ROWS][BED_COLS],
                                 uint8_t bed_status[BED_ROWS][BED_COLS],
                                 uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  for (int r = 0; r < BED_ROWS; r++)
  {
    for (int c = 0; c < BED_COLS; c++)
    {
      // outside body = very low pressure for current UI visualization
      bed_value[r][c] = 2U;
      bed_status[r][c] = 0U;
      bed_valid[r][c] = 1U;
    }
  }

  g_pattern_info.body_top_row = 0U;
  g_pattern_info.body_bottom_row = 0U;
  g_pattern_info.center_col = 8U;
}
/*----------------------------------------------------------*/


// === helper: checkerboard debug pattern ===
static void TestPattern_FillCheckerboard(uint8_t bed_value[BED_ROWS][BED_COLS],
                                         uint8_t bed_status[BED_ROWS][BED_COLS],
                                         uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  TestPattern_ClearBed(bed_value, bed_status, bed_valid);

  for (int r = 0; r < BED_ROWS; r++)
  {
    for (int c = 0; c < BED_COLS; c++)
    {
      bed_value[r][c] = ((r + c) % 2) ? 10U : 50U;
    }
  }

  g_pattern_info.body_top_row = 0U;
  g_pattern_info.body_bottom_row = 31U;
  g_pattern_info.center_col = 8U;
}

/*----------------------------------------------------------*/

// === helper: simple linear gradient debug pattern ===
static void TestPattern_FillGradient(uint8_t bed_value[BED_ROWS][BED_COLS],
                                     uint8_t bed_status[BED_ROWS][BED_COLS],
                                     uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  TestPattern_ClearBed(bed_value, bed_status, bed_valid);

  for (int r = 0; r < BED_ROWS; r++)
  {
    for (int c = 0; c < BED_COLS; c++)
    {
      bed_value[r][c] = (uint8_t)((r * BED_COLS + c) & 0x3F);
    }
  }

  g_pattern_info.body_top_row = 0U;
  g_pattern_info.body_bottom_row = 31U;
  g_pattern_info.center_col = 8U;
}

/*----------------------------------------------------------*/

static void TestPattern_Blend(uint8_t out[BED_ROWS][BED_COLS],
                              uint8_t a[BED_ROWS][BED_COLS],
                              uint8_t b[BED_ROWS][BED_COLS],
                              uint8_t alpha) // 0..255
{
  for (int r = 0; r < BED_ROWS; r++)
  {
    for (int c = 0; c < BED_COLS; c++)
    {
      uint16_t va = a[r][c];
      uint16_t vb = b[r][c];

      out[r][c] = (uint8_t)((va * (255 - alpha) + vb * alpha) / 255);
    }
  }
}
/*----------------------------------------------------------*/
/*------------- TestPattern_FillAdultAtCenter --------------*/
/*----------------------------------------------------------*/


/*----------------------------------------------------------*/
/*--------------- PATTERN_BODY_ADULT_NORMAL ----------------*/
/*----------------------------------------------------------*/

static void TestPattern_FillAdultNormal(uint8_t bed_value[BED_ROWS][BED_COLS],
                                        uint8_t bed_status[BED_ROWS][BED_COLS],
                                        uint8_t bed_valid[BED_ROWS][BED_COLS])
{
	TestPattern_FillRealisticBody(bed_value, bed_status, bed_valid,
	                              8,   // center
	                              0,   // sacrum
	                              0,   // heel L
	                              0,   // heel R
	                              0);  // scale
}

/*----------------------------------------------------------*/
/*--------------- PATTERN_BODY_SHORT_LIGHT -----------------*/
/*----------------------------------------------------------*/


static void TestPattern_FillShiftRight(uint8_t bed_value[BED_ROWS][BED_COLS],
                                       uint8_t bed_status[BED_ROWS][BED_COLS],
                                       uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  TestPattern_FillRealisticBody(bed_value, bed_status, bed_valid, 10U, 0, 0, 0, 0);
}


/*----------------------------------------------------------*/
/*--------------- PATTERN_BODY_SHIFT_LEFT -----------------*/
/*----------------------------------------------------------*/
static void TestPattern_FillShiftLeft(uint8_t bed_value[BED_ROWS][BED_COLS],
                                      uint8_t bed_status[BED_ROWS][BED_COLS],
                                      uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  TestPattern_FillRealisticBody(bed_value, bed_status, bed_valid, 6U, 0, 0, 0, 0);
}

/*----------------------------------------------------------*/
/*--------------- PATTERN_BODY_SHIFT_RIGHT -----------------*/
/*----------------------------------------------------------*/

static void TestPattern_FillShortLight(uint8_t bed_value[BED_ROWS][BED_COLS],
        								uint8_t bed_status[BED_ROWS][BED_COLS],
										uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  TestPattern_FillRealisticBody(bed_value, bed_status, bed_valid,
                                8U,
                                -5,   // sacrum کمتر ولی نه خیلی
                                -5,
                                -5,
                                -12); // 👈 فقط scale پایین
}
/*----------------------------------------------------------*/
/*------------ PATTERN_BODY_ONE_HEEL_DOMINANT --------------*/
/*----------------------------------------------------------*/

static void TestPattern_FillOneHeelDominant(uint8_t bed_value[BED_ROWS][BED_COLS],
                                            uint8_t bed_status[BED_ROWS][BED_COLS],
                                            uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  TestPattern_FillRealisticBody(bed_value, bed_status, bed_valid,
                                8U,   // center
                                0,    // sacrum boost
                                0,    // left heel
                                20,   // 👈 right heel dominant
                                0);   // body scale
}

/*----------------------------------------------------------*/
/*------------- PATTERN_BODY_SACRUM_DOMINANT ---------------*/
/*----------------------------------------------------------*/

static void TestPattern_FillSacrumDominant(uint8_t bed_value[BED_ROWS][BED_COLS],
                                           uint8_t bed_status[BED_ROWS][BED_COLS],
                                           uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  TestPattern_FillRealisticBody(bed_value, bed_status, bed_valid,
                                8U,
                                15,
                                -5,
                                -5,
                                0);
}
/*----------------------------------------------------------*/
/*-------------- PATTERN_BODY_PARTIAL_FAULT ----------------*/
/*----------------------------------------------------------*/
static void TestPattern_FillPartialFault(uint8_t bed_value[BED_ROWS][BED_COLS],
                                         uint8_t bed_status[BED_ROWS][BED_COLS],
                                         uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  TestPattern_FillRealisticBody(bed_value, bed_status, bed_valid,
                                8U, 0, 0, 0, 0);

  for (int r = 29; r <= 30; r++)
  {
    for (int c = 8; c <= 9; c++)
    {
      if (r < BED_ROWS && c < BED_COLS)
      {
        bed_value[r][c] = 8U;
        bed_status[r][c] = 2U;
        bed_valid[r][c] = 0U;
      }
    }
  }

  for (int r = 27; r <= 28; r++)
  {
    for (int c = 8; c <= 9; c++)
    {
      if (r < BED_ROWS && c < BED_COLS)
      {
        bed_status[r][c] = 1U;
        bed_valid[r][c] = 1U;
      }
    }
  }
}

/*----------------------------------------------------------*/
/*----------------- PATTERN_BODY_RESTLESS ------------------*/
/*----------------------------------------------------------*/

static void TestPattern_FillRestless(uint8_t bed_value[BED_ROWS][BED_COLS],
                                     uint8_t bed_status[BED_ROWS][BED_COLS],
                                     uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  static uint8_t phase = 0U;
  phase++;

  uint8_t center;
  switch ((phase / 8U) & 0x03U)
  {
    case 0:  center = 7U; break;
    case 1:  center = 8U; break;
    case 2:  center = 9U; break;
    default: center = 8U; break;
  }

  TestPattern_FillRealisticBody(bed_value, bed_status, bed_valid,
                                center,
                                0,
                                0,
                                0,
                                0);
}

/*----------------------------------------------------------*/
/*------------- PATTERN_BODY_REALISTIC_SUPINE --------------*/
/*----------------------------------------------------------*/

static int TestPattern_ClampInt(int x, int lo, int hi)
{
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}


// === add one smooth elliptical blob ===
// peak_value: شدت مرکز blob
// rx, ry: شعاع افقی/عمودی
static void TestPattern_AddBlob(uint8_t bed_value[BED_ROWS][BED_COLS],
                                int cx, int cy,
                                int rx, int ry,
                                int peak_value)
{
  if (rx <= 0 || ry <= 0)
    return;

  for (int r = 0; r < BED_ROWS; r++)
  {
    for (int c = 0; c < BED_COLS; c++)
    {
      int dx = c - cx;
      int dy = r - cy;

      // distance تقریبی بیضوی بدون float
      int score = (dx * dx * 100) / (rx * rx) + (dy * dy * 100) / (ry * ry);

      // فقط داخل blob اثر بده
      if (score <= 100)
      {
        // center -> peak_value
        // edge   -> حدود 0
        int add = (peak_value * (100 - score)) / 100;

        int v = bed_value[r][c] + add;
        bed_value[r][c] = (uint8_t)TestPattern_ClampInt(v, 0, 63);
      }
    }
  }
}


/*----------------------------------------------------------*/
static void TestPattern_AddSideChain(uint8_t bed_value[BED_ROWS][BED_COLS],
                                     int base_x,
                                     int mirror,
                                     int body_scale_bias)
{
  int s = mirror ? -1 : 1;

  // head
  TestPattern_AddBlob(bed_value, base_x + s * 1, 4, 2, 2, 18 + body_scale_bias / 4);

  // neck / shoulder bridge
  TestPattern_AddBlob(bed_value, base_x + s * 2, 6, 2, 2, 12 + body_scale_bias / 5);

  // shoulder
  TestPattern_AddBlob(bed_value, base_x + s * 3, 9, 3, 2, 32 + body_scale_bias / 4);

  // upper thorax
  TestPattern_AddBlob(bed_value, base_x + s * 3, 12, 3, 3, 24 + body_scale_bias / 4);

  // waist trail
  TestPattern_AddBlob(bed_value, base_x + s * 2, 15, 2, 3, 16 + body_scale_bias / 5);

  // pelvis / hip hotspot
  TestPattern_AddBlob(bed_value, base_x + s * 2, 18, 3, 3, 44 + body_scale_bias / 3);
  TestPattern_AddBlob(bed_value, base_x + s * 2, 18, 4, 4, 12);

  // thigh
  TestPattern_AddBlob(bed_value, base_x + s * 1, 23, 2, 4, 18 + body_scale_bias / 5);

  // knee
  TestPattern_AddBlob(bed_value, base_x + s * 0, 27, 2, 2, 18 + body_scale_bias / 6);

  // lower leg / heel tail
  TestPattern_AddBlob(bed_value, base_x + s * (-1), 30, 2, 2, 14 + body_scale_bias / 6);
}
/*----------------------------------------------------------*/

static void TestPattern_FillSideLying(uint8_t bed_value[BED_ROWS][BED_COLS],
                                      uint8_t bed_status[BED_ROWS][BED_COLS],
                                      uint8_t bed_valid[BED_ROWS][BED_COLS],
                                      uint8_t side_right)
{
  TestPattern_ClearBed(bed_value, bed_status, bed_valid);

  for (int r = 0; r < BED_ROWS; r++)
  {
    for (int c = 0; c < BED_COLS; c++)
    {
      bed_value[r][c] = 2U;
      bed_status[r][c] = 0U;
      bed_valid[r][c] = 1U;
    }
  }

  // right side => body near right half
  // left side  => mirrored to left half
  int base_x = side_right ? 7 : 8;
  int mirror = side_right ? 0 : 1;

  g_pattern_info.body_top_row = 2U;
  g_pattern_info.body_bottom_row = 31U;
  g_pattern_info.center_col = (uint8_t)base_x;

  TestPattern_AddSideChain(bed_value, base_x, mirror, 0);


  // neck bridge
  if (side_right)
    TestPattern_AddBlob(bed_value, 9, 6, 2, 2, 10);
  else
    TestPattern_AddBlob(bed_value, 6, 6, 2, 2, 10);

  // shoulder contact boost
  if (side_right)
    TestPattern_AddBlob(bed_value, 10, 9, 3, 2, 18);
  else
    TestPattern_AddBlob(bed_value, 5, 9, 3, 2, 18);

  // hip contact boost + falloff
  if (side_right) {
    TestPattern_AddBlob(bed_value, 11, 18, 2, 2, 16);
    TestPattern_AddBlob(bed_value, 10, 18, 4, 4, 12);
  } else {
    TestPattern_AddBlob(bed_value, 4, 18, 2, 2, 16);
    TestPattern_AddBlob(bed_value, 5, 18, 4, 4, 12);
  }

  // bent knee hint
  if (side_right)
    TestPattern_AddBlob(bed_value, 8, 26, 2, 2, 10);
  else
    TestPattern_AddBlob(bed_value, 7, 26, 2, 2, 10);

  for (int r = 0; r < BED_ROWS; r++)
  {
    for (int c = 0; c < BED_COLS; c++)
    {
      if (bed_value[r][c] < 5U)
        bed_value[r][c] = 2U;
      if (bed_value[r][c] > 63U)
        bed_value[r][c] = 63U;
    }
  }
}
/*----------------------------------------------------------*/
/*static void TestPattern_FillRealisticBody(uint8_t bed_value[BED_ROWS][BED_COLS],
                                          uint8_t bed_status[BED_ROWS][BED_COLS],
                                          uint8_t bed_valid[BED_ROWS][BED_COLS],
                                          uint8_t center,
                                          int sacrum_boost,
                                          int heel_left_boost,
                                          int heel_right_boost,
                                          int body_scale_bias)
{
  TestPattern_ClearBed(bed_value, bed_status, bed_valid);

  const uint8_t body_top = 2U;
  const uint8_t body_bottom = 31U;

  g_pattern_info.body_top_row = body_top;
  g_pattern_info.body_bottom_row = body_bottom;
  g_pattern_info.center_col = center;

  for (int r = 0; r < BED_ROWS; r++)
  {
    for (int c = 0; c < BED_COLS; c++)
    {
      bed_value[r][c] = 2U;
      bed_status[r][c] = 0U;
      bed_valid[r][c] = 1U;
    }
  }

  // head
  TestPattern_AddBlob(bed_value, center, 3, 2, 2, 16 + body_scale_bias / 4);

  // neck / upper transition
  TestPattern_AddBlob(bed_value, center, 5, 1, 1, 8 + body_scale_bias / 6);

  // shoulders / upper chest
  TestPattern_AddBlob(bed_value, center, 8, 5, 2, 34 + body_scale_bias / 3);

  // torso
  TestPattern_AddBlob(bed_value, center, 13, 4, 4, 18 + body_scale_bias / 4);

  // torso bridge
  TestPattern_AddBlob(bed_value, center, 14, 5, 6, 14 + body_scale_bias / 5);

  // sacrum / pelvis
  TestPattern_AddBlob(bed_value, center, 18, 3, 3, 55 + sacrum_boost + body_scale_bias / 3);

  // upper legs
  TestPattern_AddBlob(bed_value, center, 23, 2, 4, 16 + body_scale_bias / 5);

  // lower legs
  TestPattern_AddBlob(bed_value, center, 28, 2, 3, 10 + body_scale_bias / 6);

  // left heel
  TestPattern_AddBlob(bed_value, center - 1, 30, 1, 1, 30 + heel_left_boost);

  // right heel
  TestPattern_AddBlob(bed_value, center + 1, 30, 1, 1, 30 + heel_right_boost);

  for (int r = 0; r < BED_ROWS; r++)
  {
    for (int c = 0; c < BED_COLS; c++)
    {
      if (bed_value[r][c] < 5U)
        bed_value[r][c] = 2U;

      if (bed_value[r][c] > 63U)
        bed_value[r][c] = 63U;
    }
  }
}*/


static void TestPattern_FillRealisticBody(uint8_t bed_value[BED_ROWS][BED_COLS],
                                          uint8_t bed_status[BED_ROWS][BED_COLS],
                                          uint8_t bed_valid[BED_ROWS][BED_COLS],
                                          uint8_t center,
                                          int sacrum_boost,
                                          int heel_left_boost,
                                          int heel_right_boost,
                                          int body_scale_bias)
{
  // === clear base ===
  TestPattern_ClearBed(bed_value, bed_status, bed_valid);

  const uint8_t body_top = 2U;
  const uint8_t body_bottom = 31U;

  g_pattern_info.body_top_row = body_top;
  g_pattern_info.body_bottom_row = body_bottom;
  g_pattern_info.center_col = center;

  // === background baseline ===
  for (int r = 0; r < BED_ROWS; r++)
  {
    for (int c = 0; c < BED_COLS; c++)
    {
      bed_value[r][c] = 2U;
      bed_status[r][c] = 0U;
      bed_valid[r][c] = 1U;
    }
  }

  // ================================
  // BODY BLOBS (REALISTIC SUPINE BASE)
  // ================================

  // head
  TestPattern_AddBlob(bed_value, center, 3, 2, 2,
                      16 + body_scale_bias / 4);

  // neck (خیلی مهم برای اتصال)
  TestPattern_AddBlob(bed_value, center, 5, 1, 1,
                      8 + body_scale_bias / 6);

  // shoulders
  TestPattern_AddBlob(bed_value, center, 8, 5, 2,
                      34 + body_scale_bias / 3);

  // torso
  TestPattern_AddBlob(bed_value, center, 13, 4, 4,
                      18 + body_scale_bias / 4);

  // torso bridge (smooth transition)
  TestPattern_AddBlob(bed_value, center, 14, 5, 6,
                      14 + body_scale_bias / 5);

  // sacrum / pelvis (main hotspot)
  TestPattern_AddBlob(bed_value, center, 18, 3, 3,
                      55 + sacrum_boost + body_scale_bias / 3);

  // upper legs
  TestPattern_AddBlob(bed_value, center, 23, 2, 4,
                      16 + body_scale_bias / 5);

  // lower legs
  TestPattern_AddBlob(bed_value, center, 28, 2, 3,
                      10 + body_scale_bias / 6);

  // heels
  TestPattern_AddBlob(bed_value, center - 1, 30, 1, 1,
                      30 + heel_left_boost);

  TestPattern_AddBlob(bed_value, center + 1, 30, 1, 1,
                      30 + heel_right_boost);

  // ================================
  // CLEANUP / CLAMP
  // ================================
  for (int r = 0; r < BED_ROWS; r++)
  {
    for (int c = 0; c < BED_COLS; c++)
    {
      // حذف نویز خیلی کم
      if (bed_value[r][c] < 5U)
        bed_value[r][c] = 2U;

      // clamp
      if (bed_value[r][c] > 63U)
        bed_value[r][c] = 63U;
    }
  }
}



/*----------------------------------------------------------*/


static void TestPattern_FillRealisticSupine(uint8_t bed_value[BED_ROWS][BED_COLS],
                                            uint8_t bed_status[BED_ROWS][BED_COLS],
                                            uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  TestPattern_FillRealisticBody(bed_value, bed_status, bed_valid,
                                8U,   // center
                                0,    // sacrum boost
                                0,    // left heel boost
                                0,    // right heel boost
                                0);   // body scale bias
}

/*----------------------------------------------------------*/


static void TestPattern_FillSideLeft(uint8_t bed_value[BED_ROWS][BED_COLS],
                                     uint8_t bed_status[BED_ROWS][BED_COLS],
                                     uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  TestPattern_FillSideLying(bed_value, bed_status, bed_valid, 0U);
}

static void TestPattern_FillSideRight(uint8_t bed_value[BED_ROWS][BED_COLS],
                                      uint8_t bed_status[BED_ROWS][BED_COLS],
                                      uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  TestPattern_FillSideLying(bed_value, bed_status, bed_valid, 1U);
}

/*----------------------------------------------------------*/


static void TestPattern_FillTurningCycle(uint8_t bed_value[BED_ROWS][BED_COLS],
                                         uint8_t bed_status[BED_ROWS][BED_COLS],
                                         uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  uint32_t now_ms = HAL_GetTick();
  uint32_t phase_s = (now_ms / 1000U) % 50U;

  if (phase_s < 10U)
  {
    // 0..9s => supine
    TestPattern_FillRealisticSupine(bed_value, bed_status, bed_valid);
  }
  else if (phase_s < 20U)
  {
    // 10..19s => right side
    TestPattern_FillSideRight(bed_value, bed_status, bed_valid);
  }
  else if (phase_s < 30U)
  {
    // 20..29s => supine
    TestPattern_FillRealisticSupine(bed_value, bed_status, bed_valid);
  }
  else if (phase_s < 40U)
  {
    // 30..39s => left side
    TestPattern_FillSideLeft(bed_value, bed_status, bed_valid);
  }
  else
  {
    // 40..49s => supine
    TestPattern_FillRealisticSupine(bed_value, bed_status, bed_valid);
  }
}

/*----------------------------------------------------------*/


static void TestPattern_FillTurningCycleSmooth(uint8_t bed_value[BED_ROWS][BED_COLS],
                                               uint8_t bed_status[BED_ROWS][BED_COLS],
                                               uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  static uint8_t A[BED_ROWS][BED_COLS];
  static uint8_t B[BED_ROWS][BED_COLS];

  uint32_t now = HAL_GetTick();
  uint32_t t = (now / 1000U) % 40U;   // کل سیکل 40 ثانیه

  uint8_t alpha = 0;

  // ----------------------------
  // 0–10: supine ثابت
  // ----------------------------
  if (t < 10)
  {
    TestPattern_FillRealisticSupine(bed_value, bed_status, bed_valid);
    return;
  }

  // ----------------------------
  // 10–15: supine → right
  // ----------------------------
  if (t < 15)
  {
    TestPattern_FillRealisticSupine(A, bed_status, bed_valid);
    TestPattern_FillSideRight(B, bed_status, bed_valid);

    alpha = (uint8_t)((t - 10) * 255 / 5);
    TestPattern_Blend(bed_value, A, B, alpha);
    return;
  }

  // ----------------------------
  // 15–25: right ثابت
  // ----------------------------
  if (t < 25)
  {
    TestPattern_FillSideRight(bed_value, bed_status, bed_valid);
    return;
  }

  // ----------------------------
  // 25–30: right → supine
  // ----------------------------
  if (t < 30)
  {
    TestPattern_FillSideRight(A, bed_status, bed_valid);
    TestPattern_FillRealisticSupine(B, bed_status, bed_valid);

    alpha = (uint8_t)((t - 25) * 255 / 5);
    TestPattern_Blend(bed_value, A, B, alpha);
    return;
  }

  // ----------------------------
  // 30–35: supine → left
  // ----------------------------
  if (t < 35)
  {
    TestPattern_FillRealisticSupine(A, bed_status, bed_valid);
    TestPattern_FillSideLeft(B, bed_status, bed_valid);

    alpha = (uint8_t)((t - 30) * 255 / 5);
    TestPattern_Blend(bed_value, A, B, alpha);
    return;
  }

  // ----------------------------
  // 35–40: left → supine
  // ----------------------------
  TestPattern_FillSideLeft(A, bed_status, bed_valid);
  TestPattern_FillRealisticSupine(B, bed_status, bed_valid);

  alpha = (uint8_t)((t - 35) * 255 / 5);
  TestPattern_Blend(bed_value, A, B, alpha);
}
/*----------------------------------------------------------*/

// === generate one artificial bed snapshot ===
void TestPattern_Generate(uint8_t bed_value[BED_ROWS][BED_COLS],
                          uint8_t bed_status[BED_ROWS][BED_COLS],
                          uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  switch (TEST_PATTERN_MODE)
  {
    case PATTERN_GRADIENT_DEBUG:
      TestPattern_FillGradient(bed_value, bed_status, bed_valid);
      break;

    case PATTERN_CHECKERBOARD_DEBUG:
      TestPattern_FillCheckerboard(bed_value, bed_status, bed_valid);
      break;

    case PATTERN_BODY_ADULT_NORMAL:
      TestPattern_FillAdultNormal(bed_value, bed_status, bed_valid);
      break;
    case PATTERN_BODY_SHORT_LIGHT:
    	TestPattern_FillShortLight(bed_value, bed_status, bed_valid);
      break;
    case PATTERN_BODY_SHIFT_LEFT:
    	TestPattern_FillShiftLeft(bed_value, bed_status, bed_valid);
      break;
    case PATTERN_BODY_SHIFT_RIGHT:
    	TestPattern_FillShiftRight(bed_value, bed_status, bed_valid);
      break;
    case PATTERN_BODY_ONE_HEEL_DOMINANT:
    	TestPattern_FillOneHeelDominant(bed_value, bed_status, bed_valid);
      break;
    case PATTERN_BODY_SACRUM_DOMINANT:
    	TestPattern_FillSacrumDominant(bed_value, bed_status, bed_valid);
      break;
    case PATTERN_BODY_PARTIAL_FAULT:
    	TestPattern_FillPartialFault(bed_value, bed_status, bed_valid);
      break;
    case PATTERN_BODY_RESTLESS:
    	TestPattern_FillRestless(bed_value, bed_status, bed_valid);
      break;
    case PATTERN_BODY_REALISTIC_SUPINE:
    	TestPattern_FillRealisticSupine(bed_value, bed_status, bed_valid);
      break;
    case PATTERN_BODY_SIDE_LEFT:
        TestPattern_FillSideLeft(bed_value, bed_status, bed_valid);
        break;
    case PATTERN_BODY_TURNING_CYCLE:
       TestPattern_FillTurningCycle(bed_value, bed_status, bed_valid);
      break;

    case PATTERN_BODY_SIDE_RIGHT:
        TestPattern_FillSideRight(bed_value, bed_status, bed_valid);
        break;
    case PATTERN_BODY_TURNING_CYCLE_SMOOTH:
        TestPattern_FillTurningCycleSmooth(bed_value, bed_status, bed_valid);
      break;


    case PATTERN_REAL_DATA:
    default:
      // اگر synthetic خاموش است، این تابع نباید صدا زده شود
      // ولی برای safety چیزی تغییر نده
      break;
  }
}

/*----------------------------------------------------------*/

const TestPatternInfo_t* TestPattern_GetInfo(void)
{
  return &g_pattern_info;
}
/*----------------------------------------------------------*/

/*----------------------------------------------------------*/

/*----------------------------------------------------------*/

/*----------------------------------------------------------*/

/*----------------------------------------------------------*/
