#include "test_pattern.h"
/*----------------------------------------------------------*/
/*----------------------------------------------------------*/
/*----------------------------------------------------------*/

// === synthetic pattern metadata ===
static TestPatternInfo_t g_pattern_info;

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

// === helper: return half-width of body at a given row for adult pattern ===
static uint8_t TestPattern_AdultHalfWidth(int r)
{
  if (r < 2)   return 0U; // above body

  // head
  if (r == 2)  return 1U;
  if (r == 3)  return 2U;
  if (r == 4)  return 2U;
  if (r == 5)  return 1U;

  // shoulders
  if (r <= 9)  return 4U;

  // torso
  if (r <= 15) return 3U;

  // hip / sacrum
  if (r <= 21) return 4U;

  // legs
  if (r <= 27) return 2U;

  // heels / feet region
  if (r <= 29) return 1U;

  return 0U;
}

/*----------------------------------------------------------*/
/*------------- TestPattern_FillAdultAtCenter --------------*/
/*----------------------------------------------------------*/

static void TestPattern_FillAdultAtCenter(uint8_t bed_value[BED_ROWS][BED_COLS],
                                          uint8_t bed_status[BED_ROWS][BED_COLS],
                                          uint8_t bed_valid[BED_ROWS][BED_COLS],
                                          uint8_t center)
{
  TestPattern_ClearBed(bed_value, bed_status, bed_valid);

  const uint8_t body_top = 2U;
  const uint8_t body_bottom = 31U;

  g_pattern_info.body_top_row = body_top;
  g_pattern_info.body_bottom_row = body_bottom;
  g_pattern_info.center_col = center;

  for (int r = body_top; r <= body_bottom; r++)
  {
    uint8_t half_w = TestPattern_AdultHalfWidth(r);

    if (half_w == 0U)
      continue;

    int c0 = (int)center - (int)half_w;
    int c1 = (int)center + (int)half_w;

    for (int c = c0; c <= c1; c++)
    {
      if (c < 0 || c >= BED_COLS)
        continue;

      uint8_t val = 18U;

      // head
      if (r >= 2 && r <= 5)
        val = 20U;

      // shoulders
      if (r >= 6 && r <= 9)
        val = 34U;

      // torso
      if (r >= 10 && r <= 14)
        val = 24U;

      // sacrum / hip hotspot
      if (r >= 15 && r <= 20 && c >= ((int)center - 2) && c <= ((int)center + 2))
        val = 56U;

      // upper legs
      if (r >= 21 && r <= 24 && c >= ((int)center - 1) && c <= ((int)center + 1))
        val = 30U;

      // knees / transition
      if (r >= 25 && r <= 26 && c >= ((int)center - 1) && c <= ((int)center + 1))
        val = 24U;

      // lower legs
      if (r >= 27 && r <= 29 && c >= ((int)center - 1) && c <= ((int)center + 1))
        val = 18U;

      // feet / heel region
      if (r >= 30 && r <= 31 && c >= ((int)center - 1) && c <= ((int)center + 1))
        val = 42U;

      bed_value[r][c] = val;
      bed_status[r][c] = 0U;
      bed_valid[r][c] = 1U;
    }
  }
}


/*----------------------------------------------------------*/
/*--------------- PATTERN_BODY_ADULT_NORMAL ----------------*/
/*----------------------------------------------------------*/

static void TestPattern_FillAdultNormal(uint8_t bed_value[BED_ROWS][BED_COLS],
                                        uint8_t bed_status[BED_ROWS][BED_COLS],
                                        uint8_t bed_valid[BED_ROWS][BED_COLS])
{
	  TestPattern_FillAdultAtCenter(bed_value, bed_status, bed_valid, 8U);
}

/*----------------------------------------------------------*/
/*--------------- PATTERN_BODY_SHORT_LIGHT -----------------*/
/*----------------------------------------------------------*/


static void TestPattern_FillShortLight(uint8_t bed_value[BED_ROWS][BED_COLS],
                                       uint8_t bed_status[BED_ROWS][BED_COLS],
                                       uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  TestPattern_ClearBed(bed_value, bed_status, bed_valid);

  const uint8_t body_top = 4U;
  const uint8_t body_bottom = 26U;
  const uint8_t center = 8U;

  g_pattern_info.body_top_row = body_top;
  g_pattern_info.body_bottom_row = body_bottom;
  g_pattern_info.center_col = center;

  for (int r = body_top; r <= body_bottom; r++)
  {
    uint8_t half_w = TestPattern_AdultHalfWidth(r);

    if (half_w > 0)
        half_w -= 1;   // 👈 کل بدن باریک‌تر

    if (half_w == 0U)
      continue;

    int c0 = center - half_w;
    int c1 = center + half_w;

    for (int c = c0; c <= c1; c++)
    {
      if (c < 0 || c >= BED_COLS)
        continue;

      uint8_t val = 14U;  // 👈 کل فشار کمتر

      // head
      if (r >= 4 && r <= 6)
        val = 16U;

      // shoulders
      if (r >= 7 && r <= 10)
        val = 26U;

      // torso
      if (r >= 11 && r <= 15)
        val = 20U;

      // sacrum (کم‌تر از adult)
      if (r >= 16 && r <= 19 && c >= 7 && c <= 9)
        val = 36U;

      // upper legs
      if (r >= 20 && r <= 22 && c >= 7 && c <= 9)
        val = 22U;

      // lower legs
      if (r >= 23 && r <= 25 && c >= 7 && c <= 9)
        val = 16U;

      // heel (خیلی ضعیف‌تر)
      if (r >= 25 && r <= 26 && c >= 7 && c <= 9)
        val = 20U;

      bed_value[r][c] = val;
      bed_status[r][c] = 0U;
      bed_valid[r][c] = 1U;
    }
  }
}


/*----------------------------------------------------------*/
/*--------------- PATTERN_BODY_SHIFT_LEFT -----------------*/
/*----------------------------------------------------------*/
static void TestPattern_FillShiftLeft(uint8_t bed_value[BED_ROWS][BED_COLS],
                                      uint8_t bed_status[BED_ROWS][BED_COLS],
                                      uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  TestPattern_ClearBed(bed_value, bed_status, bed_valid);

  const uint8_t body_top = 2U;
  const uint8_t body_bottom = 31U;
  const uint8_t center = 6U;   // === بدن به چپ شیفت داده شده ===

  g_pattern_info.body_top_row = body_top;
  g_pattern_info.body_bottom_row = body_bottom;
  g_pattern_info.center_col = center;

  for (int r = body_top; r <= body_bottom; r++)
  {
    uint8_t half_w = TestPattern_AdultHalfWidth(r);

    if (half_w == 0U)
      continue;

    int c0 = (int)center - (int)half_w;
    int c1 = (int)center + (int)half_w;

    for (int c = c0; c <= c1; c++)
    {
      if (c < 0 || c >= BED_COLS)
        continue;

      // === default body contact ===
      uint8_t val = 18U;

      // head
      if (r >= 2 && r <= 5)
        val = 20U;

      // shoulders
      if (r >= 6 && r <= 9)
        val = 34U;

      // torso
      if (r >= 10 && r <= 14)
        val = 24U;

      // sacrum / hip hotspot
      if (r >= 15 && r <= 20 && c >= (center - 2) && c <= (center + 2))
        val = 56U;

      // upper legs
      if (r >= 21 && r <= 24 && c >= (center - 1) && c <= (center + 1))
        val = 30U;

      // knees / transition
      if (r >= 25 && r <= 26 && c >= (center - 1) && c <= (center + 1))
        val = 24U;

      // lower legs
      if (r >= 27 && r <= 29 && c >= (center - 1) && c <= (center + 1))
        val = 18U;

      // feet / heel region
      if (r >= 30 && r <= 31 && c >= (center - 1) && c <= (center + 1))
        val = 42U;

      bed_value[r][c] = val;
      bed_status[r][c] = 0U;
      bed_valid[r][c] = 1U;
    }
  }
}



/*----------------------------------------------------------*/
/*--------------- PATTERN_BODY_SHIFT_RIGHT -----------------*/
/*----------------------------------------------------------*/

static void TestPattern_FillShiftRight(uint8_t bed_value[BED_ROWS][BED_COLS],
                                       uint8_t bed_status[BED_ROWS][BED_COLS],
                                       uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  TestPattern_ClearBed(bed_value, bed_status, bed_valid);

  const uint8_t body_top = 2U;
  const uint8_t body_bottom = 31U;
  const uint8_t center = 10U;   // === بدن به راست شیفت داده شده ===

  g_pattern_info.body_top_row = body_top;
  g_pattern_info.body_bottom_row = body_bottom;
  g_pattern_info.center_col = center;

  for (int r = body_top; r <= body_bottom; r++)
  {
    uint8_t half_w = TestPattern_AdultHalfWidth(r);

    if (half_w == 0U)
      continue;

    int c0 = (int)center - (int)half_w;
    int c1 = (int)center + (int)half_w;

    for (int c = c0; c <= c1; c++)
    {
      if (c < 0 || c >= BED_COLS)
        continue;

      uint8_t val = 18U;

      // head
      if (r >= 2 && r <= 5)
        val = 20U;

      // shoulders
      if (r >= 6 && r <= 9)
        val = 34U;

      // torso
      if (r >= 10 && r <= 14)
        val = 24U;

      // sacrum / hip hotspot
      if (r >= 15 && r <= 20 && c >= (center - 2) && c <= (center + 2))
        val = 56U;

      // upper legs
      if (r >= 21 && r <= 24 && c >= (center - 1) && c <= (center + 1))
        val = 30U;

      // knees / transition
      if (r >= 25 && r <= 26 && c >= (center - 1) && c <= (center + 1))
        val = 24U;

      // lower legs
      if (r >= 27 && r <= 29 && c >= (center - 1) && c <= (center + 1))
        val = 18U;

      // feet / heel region
      if (r >= 30 && r <= 31 && c >= (center - 1) && c <= (center + 1))
        val = 42U;

      bed_value[r][c] = val;
      bed_status[r][c] = 0U;
      bed_valid[r][c] = 1U;
    }
  }
}

/*----------------------------------------------------------*/
/*------------ PATTERN_BODY_ONE_HEEL_DOMINANT --------------*/
/*----------------------------------------------------------*/

static void TestPattern_FillOneHeelDominant(uint8_t bed_value[BED_ROWS][BED_COLS],
                                            uint8_t bed_status[BED_ROWS][BED_COLS],
                                            uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  // === اول الگوی پایه adult normal را بساز ===
  TestPattern_FillAdultNormal(bed_value, bed_status, bed_valid);

  // === حالا فقط یک heel را dominant کن ===
  // اینجا heel راست را قوی‌تر می‌کنیم
  // اگر خواستی بعداً نسخه left هم بسازی، فقط ستون‌ها را mirror کن

  for (int r = 30; r <= 31; r++)
  {
    for (int c = 7; c <= 9; c++)
    {
      // baseline heel region از adult_normal حفظ می‌شود
      // اما heel dominant قوی‌تر می‌شود
      bed_value[r][c] = 22U;
    }
  }

  // === dominant right heel hotspot ===
  // چون UI فعلی تو bigger value = hotter می‌بیند،
  // heel dominant را با مقدار بالاتر مشخص می‌کنیم
  if (30 < BED_ROWS && 9 < BED_COLS) bed_value[30][9] = 58U;
  if (31 < BED_ROWS && 9 < BED_COLS) bed_value[31][9] = 58U;

  if (30 < BED_ROWS && 8 < BED_COLS) bed_value[30][8] = 46U;
  if (31 < BED_ROWS && 8 < BED_COLS) bed_value[31][8] = 46U;

  // === heel مقابل ضعیف‌تر بماند ===
  if (30 < BED_ROWS && 7 < BED_COLS) bed_value[30][7] = 20U;
  if (31 < BED_ROWS && 7 < BED_COLS) bed_value[31][7] = 20U;
}

/*----------------------------------------------------------*/
/*------------- PATTERN_BODY_SACRUM_DOMINANT ---------------*/
/*----------------------------------------------------------*/

static void TestPattern_FillSacrumDominant(uint8_t bed_value[BED_ROWS][BED_COLS],
                                           uint8_t bed_status[BED_ROWS][BED_COLS],
                                           uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  // === اول الگوی پایه adult normal را بساز ===
  TestPattern_FillAdultNormal(bed_value, bed_status, bed_valid);

  // === سپس sacrum را dominant کن ===
  // چون UI فعلی value بزرگ‌تر را داغ‌تر می‌بیند،
  // hotspot مرکزی را قوی‌تر می‌کنیم

  // central sacrum hotspot
  for (int r = 15; r <= 20; r++)
  {
    for (int c = 6; c <= 10; c++)
    {
      bed_value[r][c] = 60U;
    }
  }

  // کمی falloff اطراف ساکروم برای طبیعی‌تر شدن
  for (int r = 14; r <= 21; r++)
  {
    for (int c = 5; c <= 11; c++)
    {
      if (r < 0 || r >= BED_ROWS || c < 0 || c >= BED_COLS)
        continue;

      // اگر هنوز داخل hotspot اصلی نیست، اطرافش را متوسط کن
      if (!((r >= 15 && r <= 20) && (c >= 6 && c <= 10)))
      {
        if (bed_value[r][c] < 44U)
          bed_value[r][c] = 44U;
      }
    }
  }

  // heelها را عمداً ضعیف نگه دار
  for (int r = 30; r <= 31; r++)
  {
    for (int c = 7; c <= 9; c++)
    {
      bed_value[r][c] = 16U;
    }
  }

  // upper legs هم کمتر از sacrum بمانند
  for (int r = 21; r <= 24; r++)
  {
    for (int c = 7; c <= 9; c++)
    {
      bed_value[r][c] = 22U;
    }
  }
}

/*----------------------------------------------------------*/
/*-------------- PATTERN_BODY_PARTIAL_FAULT ----------------*/
/*----------------------------------------------------------*/
static void TestPattern_FillPartialFault(uint8_t bed_value[BED_ROWS][BED_COLS],
                                         uint8_t bed_status[BED_ROWS][BED_COLS],
                                         uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  // === اول الگوی پایه adult normal را بساز ===
  TestPattern_FillAdultNormal(bed_value, bed_status, bed_valid);

  // === حالا یک fault موضعی تزریق کن ===
  // اینجا heel راست را degraded می‌کنیم

  for (int r = 30; r <= 31; r++)
  {
    for (int c = 8; c <= 9; c++)
    {
      if (r < BED_ROWS && c < BED_COLS)
      {
        // value را نگه می‌داریم یا کم‌اثر می‌کنیم
        bed_value[r][c] = 8U;

        // status = ERROR
        bed_status[r][c] = 2U;

        // invalid برای پردازش
        bed_valid[r][c] = 0U;
      }
    }
  }

  // === یک warning band کوچک هم بالاتر اضافه می‌کنیم ===
  // برای اینکه UI فقط یک fault binary نبیند
  for (int r = 28; r <= 29; r++)
  {
    for (int c = 8; c <= 9; c++)
    {
      if (r < BED_ROWS && c < BED_COLS)
      {
        bed_status[r][c] = 1U;   // WARNING
        bed_valid[r][c] = 1U;    // هنوز usable
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

  // === movement cycle ===
  // 0..7   -> center 7
  // 8..15  -> center 8
  // 16..23 -> center 9
  // 24..31 -> center 8
  uint8_t center;

  switch ((phase / 8U) & 0x03U)
  {
    case 0:  center = 7U; break;
    case 1:  center = 8U; break;
    case 2:  center = 9U; break;
    default: center = 8U; break;
  }

  TestPattern_FillAdultAtCenter(bed_value, bed_status, bed_valid, center);
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


static void TestPattern_FillRealisticSupine(uint8_t bed_value[BED_ROWS][BED_COLS],
                                            uint8_t bed_status[BED_ROWS][BED_COLS],
                                            uint8_t bed_valid[BED_ROWS][BED_COLS])
{
  TestPattern_ClearBed(bed_value, bed_status, bed_valid);

  // === برای UI فعلی:
  // مقدار بیشتر = داغ‌تر
  // پس background را کم می‌گذاریم
  for (int r = 0; r < BED_ROWS; r++)
  {
    for (int c = 0; c < BED_COLS; c++)
    {
      bed_value[r][c] = 2U;
      bed_status[r][c] = 0U;
      bed_valid[r][c] = 1U;
    }
  }

  const uint8_t body_top = 2U;
  const uint8_t body_bottom = 31U;
  const uint8_t center = 8U;

  g_pattern_info.body_top_row = body_top;
  g_pattern_info.body_bottom_row = body_bottom;
  g_pattern_info.center_col = center;

  // -------------------------------------------------
  // body blobs
  // -------------------------------------------------

  // head
  TestPattern_AddBlob(bed_value, center, 3, 2, 2, 16);

  // shoulders / upper chest
  TestPattern_AddBlob(bed_value, center, 8, 5, 2, 34);

  // torso
  TestPattern_AddBlob(bed_value, center, 13, 4, 4, 18);

  // torso bridge (خیلی مهم)
  TestPattern_AddBlob(bed_value, center, 14, 5, 6, 14);

  // sacrum / pelvis hotspot (dominant)
  TestPattern_AddBlob(bed_value, center, 18, 3, 3, 55);

  // upper legs
  TestPattern_AddBlob(bed_value, center, 23, 2, 4, 16);

  // lower legs
  TestPattern_AddBlob(bed_value, center, 28, 2, 3, 10);

  // left heel
  TestPattern_AddBlob(bed_value, 7, 30, 1, 1, 30);

  // right heel
  TestPattern_AddBlob(bed_value, 9, 30, 1, 1, 30);

  // -------------------------------------------------
  // clip very low body remnants if needed
  // -------------------------------------------------
  for (int r = 0; r < BED_ROWS; r++)
  {
    for (int c = 0; c < BED_COLS; c++)
    {
      // برای تمیزتر شدن shape، مقادیر خیلی کم را background کن
      if (bed_value[r][c] < 5U)
        bed_value[r][c] = 2U;
    }
  }
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
