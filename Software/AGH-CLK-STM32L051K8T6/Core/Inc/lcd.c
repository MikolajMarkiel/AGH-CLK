#include "lcd.h"


/* Arg count, CMD, Args if any */
#if defined USE_ST7735
const uint8_t init_cmd[] = {
    0,  CMD_SLPOUT,
//  3,  CMD_FRMCTR1, 0x01, 0x2C, 0x2D,                     // Standard frame rate
//  3,  CMD_FRMCTR2, 0x01, 0x2C, 0x2D,                     // Standard frame rate
//  6,  CMD_FRMCTR3, 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D,   // Standard frame rate
    3,  CMD_FRMCTR1, 0x01, 0x01, 0x01,                     // Max
    3,  CMD_FRMCTR2, 0x01, 0x01, 0x01,                     // Max
    6,  CMD_FRMCTR3, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,   // Max frame rate
    1,  CMD_INVCTR,  0x07,
    3,  CMD_PWCTR1,  0xA2, 0x02, 0x84,
    1,  CMD_PWCTR2,  0xC5,
    2,  CMD_PWCTR3,  0x0A, 0x00,
    2,  CMD_PWCTR4,  0x8A, 0x2A,
    2,  CMD_PWCTR5,  0x8A, 0xEE,
    1,  CMD_VMCTR1,  0x0E,
    1,  CMD_INVOFF,  0x00,
    1,  CMD_COLMOD,  0x05,
    2,  CMD_CASET,   0x00, LCD_WIDTH-1,
    2,  CMD_RASET,   0x00, LCD_HEIGHT-1,
    1,  CMD_MADCTL,  LCD_ROTATION_CMD,
    16, CMD_GMCTRP1, 0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10,
    16, CMD_GMCTRN1, 0x03, 0x1d, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10,
    0,  CMD_NORON,
};
#elif defined USE_ST7789
const uint8_t init_cmd[] = {
    0,  CMD_SLPOUT,
    1,  CMD_COLMOD,  CMD_COLOR_MODE_16bit,
    5,  CMD_PORCTRL, 0x0C, 0x0C, 0x00, 0x33, 0x33,   // Standard porch
  //5,  CMD_PORCTRL, 0x01, 0x01, 0x00, 0x11, 0x11,   // Minimum porch (7% faster screen refresh rate)
    1,  CMD_GCTRL,   0x35,                           // Gate Control, Default value
    1,  CMD_VCOMS,   0x3F,                           // VCOM setting 0.725v (default 0.75v for 0x20)
    1,  CMD_LCMCTRL, 0X2C,                           // LCMCTRL, Default value
    1,  CMD_VDVVRHEN,0x01,                           // VDV and VRH command Enable, Default value
    1,  CMD_VRHS,    0x12,                           // VRH set, +-4.45v (default +-4.1v for 0x0B)
    1,  CMD_VDVS,    0x20,                           // VDV set, Default value
    1,  CMD_FRCTRL2, 0x0F,                           // Frame rate control in normal mode, Default refresh rate (60Hz)
  //1,  CMD_FRCTRL2, 0x01,                           // Frame rate control in normal mode, Max refresh rate (111Hz)
    2,  CMD_PWCTRL1, 0xA4, 0xA1,
    1,  CMD_MADCTL,  LCD_ROTATION_CMD,
    14, CMD_GMCTRP1, 0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23,
    14, CMD_GMCTRN1, 0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23,
    0,  CMD_INVON,
    0,  CMD_NORON
};
#endif

static void LCD_Update(void);
typedef struct{
  int8_t spi_sz;
  int8_t dma_sz;
  int8_t dma_mem_inc;
  uint8_t mode;
}config_t;

config_t config = {
    .spi_sz = -1,
    .dma_sz = -1,
    .dma_mem_inc = -1,
	.mode = 0, //mode_write
};

#ifdef LCD_LOCAL_FB
static uint16_t fb[LCD_WIDTH*LCD_HEIGHT];
#endif

static UG_GUI gui;
static UG_DEVICE device = {
    .x_dim = LCD_WIDTH,
    .y_dim = LCD_HEIGHT,
#ifdef LCD_LOCAL_FB
    .pset = LCD_DrawPixelFB,
#else
    .pset = LCD_DrawPixel,
#endif
    .flush = LCD_Update,
};

#define mode_16bit    1
#define mode_8bit     0

#define MODE_READ	1
#define MODE_WRITE	0
/*
 * @brief Sets SPI interface word size (0=8bit, 1=16 bit)
 * @param none
 * @return none
 */



static void setSPI_Size(int8_t size){
  if(config.spi_sz!=size){
    __HAL_SPI_DISABLE(&LCD_HANDLE);
    config.spi_sz=size;
    if(size==mode_16bit){
      LCD_HANDLE.Init.DataSize = SPI_DATASIZE_16BIT;
      LCD_HANDLE.Instance->CR1 |= SPI_CR1_DFF;
    }
    else{
      LCD_HANDLE.Init.DataSize = SPI_DATASIZE_8BIT;
      LCD_HANDLE.Instance->CR1 &= ~(SPI_CR1_DFF);
    }
  }
}


#ifdef USE_DMA
#define DMA_min_Sz    64
#define mem_increase  1
#define mem_fixed     0

/**
 * @brief Configures DMA/ SPI interface
 * @param memInc Enable/disable memory address increase
 * @param mode16 Enable/disable 16 bit mode (disabled = 8 bit)
 * @return none
 */
static void setDMAMemMode(uint8_t memInc, uint8_t size)
{
  setSPI_Size(size);
  if(config.dma_sz!=size || config.dma_mem_inc!=memInc){
    config.dma_sz =size;
    config.dma_mem_inc = memInc;
    __HAL_DMA_DISABLE(LCD_HANDLE.hdmatx);;
#ifdef DMA_SxCR_EN
    while((LCD_HANDLE.hdmatx->Instance->CR & DMA_SxCR_EN) != RESET);
#elif defined DMA_CCR_EN
    while((LCD_HANDLE.hdmatx->Instance->CCR & DMA_CCR_EN) != RESET);
#endif
    if(memInc==mem_increase){
      LCD_HANDLE.hdmatx->Init.MemInc = DMA_MINC_ENABLE;
#ifdef DMA_SxCR_EN
      LCD_HANDLE.hdmatx->Instance->CR |= DMA_SxCR_MINC;
#elif defined DMA_CCR_EN
      LCD_HANDLE.hdmatx->Instance->CCR |= DMA_CCR_MINC;
#endif
    }
    else{
      LCD_HANDLE.hdmatx->Init.MemInc = DMA_MINC_DISABLE;
#ifdef DMA_SxCR_EN
      LCD_HANDLE.hdmatx->Instance->CR &= ~(DMA_SxCR_MINC);
#elif defined DMA_CCR_EN
      LCD_HANDLE.hdmatx->Instance->CCR &= ~(DMA_CCR_MINC);
#endif
    }

    if(size==mode_16bit){
      LCD_HANDLE.hdmatx->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
      LCD_HANDLE.hdmatx->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
#ifdef DMA_SxCR_EN
      LCD_HANDLE.hdmatx->Instance->CR = (LCD_HANDLE.hdmatx->Instance->CR & ~(DMA_SxCR_PSIZE_Msk | DMA_SxCR_MSIZE_Msk)) |
                                                   (1<<DMA_SxCR_PSIZE_Pos | 1<<DMA_SxCR_MSIZE_Pos);
#elif defined DMA_CCR_EN
      LCD_HANDLE.hdmatx->Instance->CCR = (LCD_HANDLE.hdmatx->Instance->CCR & ~(DMA_CCR_PSIZE_Msk | DMA_CCR_MSIZE_Msk)) |
                                                   (1<<DMA_CCR_PSIZE_Pos | 1<<DMA_CCR_MSIZE_Pos);
#endif

    }
    else{
      LCD_HANDLE.hdmatx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
      LCD_HANDLE.hdmatx->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
#ifdef DMA_SxCR_EN
      LCD_HANDLE.hdmatx->Instance->CR = (LCD_HANDLE.hdmatx->Instance->CR & ~(DMA_SxCR_PSIZE_Msk | DMA_SxCR_MSIZE_Msk));
#elif defined DMA_CCR_EN
      LCD_HANDLE.hdmatx->Instance->CCR = (LCD_HANDLE.hdmatx->Instance->CCR & ~(DMA_CCR_PSIZE_Msk | DMA_CCR_MSIZE_Msk));
#endif
    }
  }
}
#endif

static uint8_t setSPI_Mode(uint8_t mode)
{
	static uint8_t modeCmd[2] = {CMD_COLMOD, CMD_COLOR_MODE_16bit};
	if (config.mode == mode){	//mode is already set
		return 0;	//mode set correctly
	} else if (mode == MODE_READ){
		uint8_t cmdread = CMD_RAMRD;
		modeCmd[1] = CMD_COLOR_MODE_18bit;
		LCD_WriteCommand(modeCmd, 1);
		LCD_WriteCommand(&cmdread, 0);
		HAL_SPI_MspDeInit(&LCD_HANDLE);
		//
		GPIO_InitTypeDef GPIO_InitStruct = {0};
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10|GPIO_PIN_12, GPIO_PIN_RESET);
		//
		GPIO_InitStruct.Pin = GPIO_PIN_10;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
		//
		GPIO_InitStruct.Pin = GPIO_PIN_12;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	} else if (mode == MODE_WRITE){
		LCD_PIN(LCD_CS, SET);
		HAL_SPI_MspInit(&LCD_HANDLE);
		LCD_PIN(LCD_CS, RESET);
		modeCmd[1] = CMD_COLOR_MODE_16bit;
		LCD_WriteCommand(modeCmd, 1);
	} else {
		return 1; //wrong mode value
	}
	config.mode = mode;
	return 0;	//mode set correctly
}


/**
 * @brief Write command to ST7735 controller
 * @param cmd -> command to write
 * @return none
 */
void LCD_WriteCommand(uint8_t *cmd, uint8_t argc)
{
	setSPI_Size(mode_8bit);
	LCD_PIN(LCD_DC,RESET);
#ifdef LCD_CS
	LCD_PIN(LCD_CS,RESET);
#endif
	HAL_SPI_Transmit(&LCD_HANDLE, cmd, 1, HAL_MAX_DELAY);
	if(argc){
		LCD_PIN(LCD_DC,SET);
		HAL_SPI_Transmit(&LCD_HANDLE, (cmd+1), argc, HAL_MAX_DELAY);
	}
#ifdef LCD_CS
	LCD_PIN(LCD_CS,SET);
#endif
}

static uint8_t LCD_GetRed(uint16_t red){
	return (red & 0xf800)>>11;
}

static uint8_t LCD_GetGreen(uint16_t green){
	return (green & 0x07e0)>>5;
}

static uint8_t LCD_GetBlue(uint16_t blue){
	return blue & 0x001F;
}



/**
 * @brief Write data to ST7735 controller
 * @param buff -> pointer of data buffer
 * @param buff_size -> size of the data buffer
 * @return none
 */
static void LCD_WriteData(uint8_t *buff, size_t buff_size)
{
  LCD_PIN(LCD_DC,SET);
#ifdef LCD_CS
  LCD_PIN(LCD_CS,RESET);
#endif

  // split data in small chunks because HAL can't send more than 64K at once

  while (buff_size > 0) {
    uint16_t chunk_size = buff_size > 65535 ? 65535 : buff_size;
#ifdef USE_DMA
    if(buff_size>DMA_min_Sz){
      HAL_SPI_Transmit_DMA(&LCD_HANDLE, buff, chunk_size);
      while(HAL_DMA_GetState(LCD_HANDLE.hdmatx)!=HAL_DMA_STATE_READY);
      if(config.dma_mem_inc==mem_increase){
        if(config.dma_sz==mode_16bit)
          buff += chunk_size;
        else
          buff += chunk_size*2;
      }
    }
    else{
      HAL_SPI_Transmit(&LCD_HANDLE, buff, chunk_size, HAL_MAX_DELAY);
      if(config.spi_sz==mode_16bit)
        buff += chunk_size;
      else
        buff += chunk_size*2;
    }
#else
    HAL_SPI_Transmit(&LCD_HANDLE, buff, chunk_size, HAL_MAX_DELAY);
#endif
    buff_size -= chunk_size;
  }
#ifdef LCD_CS
  LCD_PIN(LCD_CS,SET);
#endif
}

/**
 * @brief Write data to ST7735 controller, simplify for 8bit data.
 * data -> data to write
 * @return none
 */

uint8_t LCD_ReadData(LCD_BUFFER* buf, uint16_t x, uint16_t y){
	//TODO: check if x0,x1,y0,y1 lie inside LCD resolution
	uint16_t *ptr = buf->p;
	if(x + buf->width >= LCD_WIDTH || y + buf->height >= LCD_HEIGHT){	//0-239
		return 1;	//wrong params
	}
	LCD_SetAddressWindow(x, y, x + buf->width, y + buf->height);
	setSPI_Mode(MODE_READ);
	 //skip first data bits
	 uint8_t i,j,m;
	 uint32_t temp;
	 for (i=0; i<16; i++){
		 HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_10);
	 }
	 //read data
	 uint8_t cnt = 0;
	 for (i=0; i<=buf->width; i++){
		 for (j=0; j<=buf->height; j++){
			 for(m=0; m<24; m++){
				temp = (temp << 1) + HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_12);
				HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_10);
				HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_10);

			 }
			 *ptr = ((temp & 0x00F80000) >> 8) | ((temp & 0x0000FC00) >> 5) | ((temp & 0x000000F8) >> 3);
			 cnt++;
			 ptr++;
		 }
	 }
	 setSPI_Mode(MODE_WRITE);
	 return 0;
}

uint16_t LCD_ColorOpacity(uint16_t col, uint16_t op){
	if 		(LCD_GetRed(col) >= op) col -= op << 11;
	else if (LCD_GetRed(col) > 0) col &= 0x07FF;

	if 		(LCD_GetGreen(col) >= op << 1) col -= op << 6;
	else if (LCD_GetGreen(col) > 0) col &= 0xF81F;

	if 		(LCD_GetBlue(col) >= op) col -= op;
	else if (LCD_GetBlue(col) > 0) col &= 0xFF80;
	return col;
}

//static void LCD_ReadCmd(uint8_t cmd, uint8_t *data, uint8_t count)
//{
//  setSPI_Size(mode_8bit);
////  LCD_PIN(LCD_CS,RESET);
////  LCD_PIN(LCD_DC,RESET);
//  HAL_SPI_Transmit(&LCD_HANDLE, &cmd, sizeof(cmd), HAL_MAX_DELAY);
////  LCD_PIN(LCD_DC,SET);
//  HAL_SPI_Receive(&LCD_HANDLE, data, count, HAL_MAX_DELAY);
////  LCD_PIN(LCD_CS,SET);
//}


/**
 * @brief Set the rotation direction of the display
 * @param m -> rotation parameter(please refer it in ST7735.h)
 * @return none
 */
void LCD_SetRotation(uint8_t m)
{
  uint8_t cmd[] = { CMD_MADCTL, 0};

  m = m % 4; // can't be higher than 3

  switch (m)
  {
  case 0:
#if LCD_IS_160X80
    cmd[1] = CMD_MADCTL_MX | CMD_MADCTL_MY | CMD_MADCTL_BGR;
#else
    cmd[1] = CMD_MADCTL_MX | CMD_MADCTL_MY | CMD_MADCTL_RGB;
#endif
    break;
  case 1:
#if CMD_IS_160X80
    cmd[1] = CMD_MADCTL_MY | CMD_MADCTL_MV | CMD_MADCTL_BGR;
#else
    cmd[1] = CMD_MADCTL_MY | CMD_MADCTL_MV | CMD_MADCTL_RGB;
#endif
    break;
  case 2:
#if CMD_IS_160X80
    cmd[1] = CMD_MADCTL_BGR;
#else
    cmd[1] = CMD_MADCTL_RGB;
#endif
    break;
  case 3:
#if CMD_IS_160X80
    cmd[1] = CMD_MADCTL_MX | CMD_MADCTL_MV | CMD_MADCTL_BGR;
#else
    cmd[1] = CMD_MADCTL_MX | CMD_MADCTL_MV | CMD_MADCTL_RGB;
#endif
    break;
  }
  LCD_WriteCommand(cmd, sizeof(cmd)-1);
}


/**
 * @brief Set address of DisplayWindow
 * @param xi&yi -> coordinates of window
 * @return none
 */
void LCD_SetAddressWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
  int16_t x_start = x0 + LCD_X_SHIFT, x_end = x1 + LCD_X_SHIFT;
  int16_t y_start = y0 + LCD_Y_SHIFT, y_end = y1 + LCD_Y_SHIFT;

  /* Column Address set */
  {
    uint8_t cmd[] = { CMD_CASET, x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF };
    LCD_WriteCommand(cmd, sizeof(cmd)-1);
  }
  /* Row Address set */
  {
    uint8_t cmd[] = { CMD_RASET, y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF };
    LCD_WriteCommand(cmd, sizeof(cmd)-1);
  }
  {
  /* Write to RAM */
    uint8_t cmd[] = { CMD_RAMWR };
    LCD_WriteCommand(cmd, sizeof(cmd)-1);
  }
}


/**
 * @brief Address and draw a Pixel
 * @param x&y -> coordinate to Draw
 * @param color -> color of the Pixel
 * @return none
 */
void LCD_DrawPixel(int16_t x, int16_t y, uint16_t color)
{
  if ((x < 0) || (x > LCD_WIDTH-1) ||
     (y < 0) || (y > LCD_HEIGHT-1))
    return;

  uint8_t data[2] = {color >> 8, color & 0xFF};

  LCD_SetAddressWindow(x, y, x, y);

  LCD_PIN(LCD_DC,SET);
#ifdef LCD_CS
  LCD_PIN(LCD_CS,RESET);
#endif
  HAL_SPI_Transmit(&LCD_HANDLE, data, sizeof(data), HAL_MAX_DELAY);
#ifdef LCD_CS
  LCD_PIN(LCD_CS,SET);
#endif
}



#ifdef LCD_LOCAL_FB
void LCD_DrawPixelFB(int16_t x, int16_t y, uint16_t color)
{
  if ((x < 0) || (x >= LCD_WIDTH) ||
     (y < 0) || (y >= LCD_HEIGHT)) return;

  fb[x+(y*LCD_WIDTH)] = color;
}
#endif

void LCD_FillPixels(uint16_t pixels, uint16_t color){
#ifdef USE_DMA
  if(pixels>DMA_min_Sz)
    LCD_WriteData((uint8_t*)&color, pixels);
  else{
#endif
    uint16_t fill[DMA_min_Sz];                                                                // Use a 64 pixel (128Byte) buffer for faster filling
    for(uint8_t t=0;t<(pixels<DMA_min_Sz ? pixels : DMA_min_Sz);t++){                                 // Fill the buffer with the color
      fill[t]=color;
    }
    while(pixels){                                                                    // Send 64 pixel blocks
      uint8_t sz = (pixels<DMA_min_Sz ? pixels : DMA_min_Sz);
      LCD_WriteData((uint8_t*)fill, sz);
      pixels-=sz;
    }
#ifdef USE_DMA
  }
#endif
}

/**
 * @brief Set address of DisplayWindow and returns raw pixel draw for uGUI driver acceleration
 * @param xi&yi -> coordinates of window
 * @return none
 */
void(*LCD_FillArea(int16_t x0, int16_t y0, int16_t x1, int16_t y1))(uint16_t,uint16_t){
  if(x0==-1){
	return NULL;
  }
#ifdef USE_DMA
    setDMAMemMode(mem_increase, mode_8bit);
#else
    setSPI_Size(mode_8bit);                                                          // Set SPI to 8 bit
#endif

  LCD_SetAddressWindow(x0,y0,x1,y1);
#ifdef USE_DMA
    setDMAMemMode(mem_fixed, mode_16bit);
#else
    setSPI_Size(mode_16bit);                                                          // Set SPI to 16 bit
#endif
  LCD_PIN(LCD_DC,SET);
  return LCD_FillPixels;
}


/**
 * @brief Fill an Area with single color
 * @param xSta&ySta -> coordinate of the start point
 * @param xEnd&yEnd -> coordinate of the end point
 * @param color -> color to Fill with
 * @return none
 */
int8_t LCD_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color)
{
  uint16_t pixels = (xEnd-xSta+1)*(yEnd-ySta+1);
  LCD_SetAddressWindow(xSta, ySta, xEnd, yEnd);
#ifdef USE_DMA
    setDMAMemMode(mem_fixed, mode_16bit);
#else
    setSPI_Size(mode_16bit);
#endif
  LCD_FillPixels(pixels, color);
#ifdef USE_DMA
  setDMAMemMode(mem_increase, mode_8bit);
#else
  setSPI_Size(mode_8bit);
#endif
  return UG_RESULT_OK;
}


/**
 * @brief Draw an Image on the screen
 * @param x&y -> start point of the Image
 * @param w&h -> width & height of the Image to Draw
 * @param data -> pointer of the Image array
 * @return none
 */
void LCD_DrawImage(uint16_t x, uint16_t y, UG_BMP* bmp)
{
  uint16_t w = bmp->width;
  uint16_t h = bmp->height;
  if ((x > LCD_WIDTH-1) || (y > LCD_HEIGHT-1))
    return;
  if ((x + w - 1) > LCD_WIDTH-1)
    return;
  if ((y + h - 1) > LCD_HEIGHT-1)
    return;
  if(bmp->bpp!=BMP_BPP_16)
    return;
  LCD_SetAddressWindow(x, y, x + w - 1, y + h - 1);

#ifdef USE_DMA
  setDMAMemMode(mem_increase, mode_16bit);                                                            // Set SPI and DMA to 16 bit, enable memory increase
#else
  setSPI_Size(mode_16bit);                                                                            // Set SPI to 16 bit
#endif
  LCD_WriteData((uint8_t*)bmp->p, w*h);
#ifdef USE_DMA
setDMAMemMode(mem_increase, mode_8bit);                                                            // Set SPI and DMA to 16 bit, enable memory increase
#else
setSPI_Size(mode_8bit);                                                                            // Set SPI to 16 bit
#endif
  }



/**
 * @brief Accelerated line draw using filling (Only for vertical/horizontal lines)
 * @param x1&y1 -> coordinate of the start point
 * @param x2&y2 -> coordinate of the end point
 * @param color -> color of the line to Draw
 * @return none
 */
int8_t LCD_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {

  if(x0==x1){                                   // If horizontal
    if(y0>y1) swap(y0,y1);
  }
  else if(y0==y1){                              // If vertical
    if(x0>x1) swap(x0,x1);
  }
  else{                                         // Else, return fail, draw using software
    return UG_RESULT_FAIL;
  }

  LCD_Fill(x0,y0,x1,y1,color);               // Draw using acceleration
  return UG_RESULT_OK;
}
void LCD_PutChar(uint16_t x, uint16_t y, char ch, UG_FONT* font, uint16_t color, uint16_t bgcolor){
  UG_FontSelect(font);
  UG_PutChar(ch, x, y, color, bgcolor);
}

void LCD_PutStr(uint16_t x, uint16_t y,  char *str, UG_FONT* font, uint16_t color, uint16_t bgcolor){
  UG_FontSelect(font);
  UG_SetForecolor(color);
  UG_SetBackcolor(bgcolor);
  UG_PutString(x, y, str);
}
/**
 * @brief Invert Fullscreen color
 * @param invert -> Whether to invert
 * @return none
 */
void LCD_InvertColors(uint8_t invert)
{
  uint8_t cmd[] = { (invert ? CMD_INVON /* INVON */ : CMD_INVOFF /* INVOFF */) };
  LCD_WriteCommand(cmd, sizeof(cmd)-1);
}

/*
 * @brief Open/Close tearing effect line
 * @param tear -> Whether to tear
 * @return none
 */
void LCD_TearEffect(uint8_t tear)
{
  uint8_t cmd[] = { (tear ? 0x35 /* TEON */ : 0x34 /* TEOFF */) };
  LCD_WriteCommand(cmd, sizeof(cmd)-1);
}

void LCD_setPower(uint8_t power)
{
  uint8_t cmd[] = { (power ? CMD_DISPON /* TEON */ : CMD_DISPOFF /* TEOFF */) };
  LCD_WriteCommand(cmd, sizeof(cmd)-1);
}

static void LCD_Update(void)
{
#ifdef LCD_LOCAL_FB
  LCD_SetAddressWindow(0,0,LCD_WIDTH-1,LCD_HEIGHT-1);
  #ifdef USE_DMA
  setDMAMemMode(mem_increase, mode_16bit);                                                            // Set SPI and DMA to 16 bit, enable memory increase
  #else
  setSPI_Size(mode_16bit);                                                                            // Set SPI to 16 bit
  #endif
  LCD_WriteData((uint8_t*)fb, LCD_WIDTH*LCD_HEIGHT);
#endif
  #ifdef USE_DMA
  setDMAMemMode(mem_increase, mode_8bit);                                                            // Set SPI and DMA to 16 bit, enable memory increase
  #else
  setSPI_Size(mode_8bit);                                                                            // Set SPI to 16 bit
  #endif
}
/**
 * @brief Initialize ST7735 controller
 * @param none
 * @return none
 */

void LCD_init(void)
{
#ifdef LCD_CS
  LCD_PIN(LCD_CS,RESET);
#endif
#ifdef LCD_RST
  LCD_PIN(LCD_RST,RESET);
  HAL_Delay(1);
  LCD_PIN(LCD_RST,SET);
  HAL_Delay(200);
#endif
  UG_Init(&gui, &device);
#ifndef LCD_LOCAL_FB
//  UG_DriverRegister(DRIVER_DRAW_LINE, LCD_DrawLine);
  UG_DriverRegister(DRIVER_FILL_FRAME, LCD_Fill);
  UG_DriverRegister(DRIVER_FILL_AREA, LCD_FillArea);
  UG_DriverRegister(DRIVER_DRAW_BMP, LCD_DrawImage);
#endif
  UG_FontSetHSpace(0);
  UG_FontSetVSpace(0);
  for(uint16_t i=0; i<sizeof(init_cmd); ){
    LCD_WriteCommand((uint8_t*)&init_cmd[i+1], init_cmd[i]);
    i += init_cmd[i]+2;
  }
//  UG_FillScreen(C_GOLD);               //  Clear screen
//  UG_FillScreen(C_WHITE);               //  Clear screen
    UG_FillScreen(C_BLACK);               //  Clear screen
  LCD_setPower(ENABLE);
  UG_Update();
}


/**
 * @brief Draw saved buffer on the screen. Buffer can be generated for example by using ReadData(...)
 * @param x&y -> start point
 * @param w&h -> width & height of the buffer to draw
 * @param buff -> pointer of the buffer array
 * @return none
 */
void LCD_DrawBuffer(uint16_t x, uint16_t y, LCD_BUFFER* buff)
{
  uint16_t w = buff->width;
  uint16_t h = buff->height;
  uint8_t *ptr_8 = buff->p;
  uint16_t *ptr_16 = buff->p;

  if ((x + w) >= LCD_WIDTH || (y + h) >= LCD_HEIGHT){
    return;
  }
  uint16_t i;
  for (i=0; i<=(buff->width+1)*(buff->height+1); i++){	//check
	  *ptr_16 = __REV16(*ptr_16);
	  ptr_16++;
  }
  LCD_SetAddressWindow(x, y, x + w, y + h);		//w&h - 0-239
#ifdef USE_DMA
  setDMAMemMode(mem_increase, mode_16bit);                                                            // Set SPI and DMA to 16 bit, enable memory increase
#else
  setSPI_Size(mode_16bit);                                                                            // Set SPI to 16 bit
#endif
  LCD_WriteData(ptr_8, (w+1)*(h+1));	//if 0, then null - min 1, max NA -> min 1*1
#ifdef USE_DMA
setDMAMemMode(mem_increase, mode_8bit);                                                            // Set SPI and DMA to 16 bit, enable memory increase
#else
setSPI_Size(mode_8bit);                                                                            // Set SPI to 16 bit
#endif
  }

uint8_t LCD_RedrawFrame(uint8_t srcX, uint8_t  srcY, uint8_t desX, uint8_t desY, uint8_t width, uint8_t height){
	uint16_t w = width; //0-240 if 0 then null
	uint16_t h = height;
	if(srcX > LCD_WIDTH) srcX = LCD_WIDTH -1; //max 239
	if(srcY > LCD_HEIGHT) srcY = LCD_HEIGHT -1; //max 239
	if(desX > LCD_WIDTH) desX = LCD_WIDTH -1; //max 239
	if(desY > LCD_HEIGHT) desY = LCD_HEIGHT -1; //max 239

	if(w + srcX > LCD_WIDTH) w = LCD_WIDTH - srcX;
	if(h + srcY > LCD_HEIGHT) h = LCD_HEIGHT - srcY;
	if(w + desX > LCD_WIDTH) w = LCD_WIDTH - desX;
	if(h + desY > LCD_HEIGHT) h = LCD_HEIGHT - desY;
	if (w==0 || h==0)
		return 1;	//empty buffer
	uint16_t* buff = (uint16_t*)malloc(w * h * sizeof(uint16_t));
	LCD_BUFFER buffer = {.p=buff, .width=w-1, .height=h-1}; // założenie 0-239
	if(LCD_ReadData(&buffer, srcX, srcY))
		return 2;	//read data error
//	Available func:

	// change red to purple
//	uint16_t i;
//	for(i=0; i<width*height; i++){
//		if (*buff > 0xD000 && (*buff&0b0000011111100000) < 0x0500){
//			*buff=*buff|0b0000000000011111;
//		}
//		buff++;
//	}
	//opacity
//	uint16_t i;
//	uint16_t *b = buff;
//	for(i=0; i<w*h; i++){
//		uint16_t opp = 3;
//		uint16_t col;
//
//		col = LCD_GetRed(*b);
//		if (col >= opp) *b -= opp << 11;
//		else if (col > 0) *b &= 0x07FF;
//
//		col = LCD_GetGreen(*b);
//		if (col >= opp<<1) *b -= opp << 6;
//		else if (col > 0) *b &= 0xF81F;
//
//		col = LCD_GetBlue(*b);
//		if (col >= opp)	*b -= opp;
//		else if (col > 0) *b &= 0xFF80;
//
//		b++;
//	}
//
	//blur
	//odczytaj buff1
	//sprawdz czy graniczne wartosci buff1 są wieksze od 0
//	uint16_t* buff2 = (uint16_t*)malloc(w * h * sizeof(uint16_t));
//	uint16_t i,j;
//	uint16_t *b = buff;
//	uint16_t *b2 = buff2;
//
//	for(i=0; i<w*h; i++){
//		*b2 = 0;
//		b2++;
//	}
//	b2 = buff2;
//	for(i=0; i<w; i++){
//		for(j=0; j<h; j++){
//
//			// get colorTotal;
//			int8_t col, row;
//			for (row=-1; row<=1; row++){
//				for (col=-1; col<=1; col++){
//					uint16_t currentX = i + col;
//					uint16_t currentY = j + col;
//					if ( currentX >= 0 && currentX < w && currentY >=0 && currentY <h){
//						//uint32 color = m_buffer2[currentY*Screen_Width+currentX]
//					}
//				}
//			}
//		}
//		*b2 = *b1;
//		b2++;
//		b1++;
//	}

	LCD_DrawBuffer(desX, desY, &buffer);	//buffer with w&h=0-239
	free(buff);
	return 0;
}

LCD_BUFFER lcd_buffer = {
	.p = NULL,
	.height = 0,
	.width = 0,
	.xref = 0,
	.yref = 0
};

void Buffer_DrawPx(int16_t x_val, int16_t y_val, uint16_t color)
{
	if (lcd_buffer.p == NULL)
		return;
	int16_t x = x_val + lcd_buffer.xref;
	int16_t y = y_val + lcd_buffer.yref;
	uint16_t w = lcd_buffer.width;
	uint16_t h = lcd_buffer.height;
	if ((x < 0) || (x > w-1) || (y < 0) || (y > h-1))
		return;
	uint16_t* ptr = lcd_buffer.p;
	ptr += y*w + x;
	*ptr = color;
}

void Buffer_Resize(uint16_t width, uint16_t height, int16_t xref, int16_t yref){
	// the same parameters
	if (width == lcd_buffer.width && height == lcd_buffer.height && xref == lcd_buffer.xref && yref == lcd_buffer.yref){
		return;
	}
	// wrong parameters
	if (width == 0 || height == 0)
		return;
	if (xref >= width || -xref > width)
		return;
	if (yref >= height || -yref > height)
		return;
	// empty buffer
	uint16_t* tempBuf = malloc(width * height * sizeof lcd_buffer.p);
	uint16_t i;
	uint16_t* ptr = tempBuf;
	for(i = 0; i < (width)*(height); i++){
		*(ptr+i) = 0;
	}
	if (lcd_buffer.p != NULL){
		uint16_t x,y;
		uint16_t* ptr;
		int16_t xoffset, yoffset;
		xoffset = xref - lcd_buffer.xref;
		yoffset = yref - lcd_buffer.yref;
		for(y=0; y<height; y++){
			if (y-yoffset < 0 || y-yoffset >= lcd_buffer.height) continue;
			for(x=0; x<width; x++){
				if (x-xoffset < 0 || x-xoffset >= lcd_buffer.width) continue;
				uint16_t* oldptr = lcd_buffer.p;
				oldptr += (y-yoffset)*lcd_buffer.width + (x-xoffset);
				ptr = tempBuf + y*width + x;
				*ptr = *oldptr;
			}
		}
		free(lcd_buffer.p);
	}
	lcd_buffer.p = tempBuf;
	lcd_buffer.height = height;
	lcd_buffer.width = width;
	lcd_buffer.xref = xref;
	lcd_buffer.yref = yref;
}

void Buffer_DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t col){
	void* psetBP = device.pset;
	device.pset = Buffer_DrawPx;
	uint16_t xmin = x1 > x2 ? x2 : x1;
	uint16_t xmax = x1 > x2 ? x1 : x2;
	uint16_t ymin = y1 > y2 ? y2 : y1;
	uint16_t ymax = y1 > y2 ? y1 : y2;
	uint16_t newHeight = lcd_buffer.height;
	uint16_t newWidth = lcd_buffer.width;
	int16_t newxref = lcd_buffer.xref;
	int16_t newyref = lcd_buffer.yref;


	if (lcd_buffer.xref + xmin < 0){
		newxref += -(lcd_buffer.xref + xmin);
		newWidth += -(lcd_buffer.xref + xmin);
	}
	if (lcd_buffer.xref + xmax > lcd_buffer.width){
		newWidth += lcd_buffer.xref + xmax - lcd_buffer.width +1;	//at least 1 px
	}
	if (lcd_buffer.yref + ymin < 0){
		newyref += -(lcd_buffer.yref + ymin);
		newHeight += -(lcd_buffer.yref + ymin);
	}
	if (lcd_buffer.yref + ymax > lcd_buffer.height){
		newHeight += lcd_buffer.yref + ymax - lcd_buffer.height +1; // at least 1 px
	}
	Buffer_Resize(newWidth, newHeight, newxref, newyref);	//if there is no changes then first condition return from Buffer_Resize()

	UG_DrawLine(x1, y1, x2, y2, col);
	device.pset = psetBP;
}
void Buffer_BoxBlur(){
	uint16_t* ptr = lcd_buffer.p;
	uint16_t w = lcd_buffer.width;
	uint16_t h = lcd_buffer.height;
	int16_t xref = lcd_buffer.xref;
	int16_t yref = lcd_buffer.yref;

	uint16_t newWidth = w;
	uint16_t newHeight = h;
	int16_t newxref = xref;
	int16_t newyref = yref;
	uint8_t x,y;
	for (x=0; x<w; x++){ // down
		if (ptr[(w-1)*h + x] != 0 ){
			newHeight++;
			break;
		}
	}
	for (x=0; x<w; x++){ // up
		if (ptr[x] != 0 ){
			newHeight++;
			newyref++;
			break;
		}
	}
	for (y=0; y<h; y++){ // right
		if (ptr[y*w] != 0 ){
			newWidth++;
			newxref++;
			break;
		}
	}
	for (y=0; y<w; y++){ // left
		if (ptr[y*w + w-1] != 0 ){
			newWidth++;
			break;
		}
	}

	Buffer_Resize(newWidth, newHeight, newxref, newyref);
	ptr = lcd_buffer.p;
	w = lcd_buffer.width;
	h = lcd_buffer.height;
	xref = lcd_buffer.xref;
	yref = lcd_buffer.yref;
	uint16_t* temp = malloc(newWidth*newHeight* sizeof temp);
	for(y = 0; y < h; y++){
		for (x=0; x < w; x++){
			temp[y*w + x] = ptr[y*w+x];
		}
	}
	for(y = 0; y < h; y++){
		for (x = 0; x < w; x++){
			int8_t row,col;

			uint32_t redTotal = 0;
			uint32_t greenTotal = 0;
			uint32_t blueTotal = 0;


			for (row = -1; row<=1; row++){
				for (col = -1; col<=1; col++){
					uint8_t currentX = x + col;
					uint8_t currentY = y + row;
					if (currentX >= 0 && currentX < w && currentY >= 0 && currentY < h){
						uint16_t color = temp[currentY*h + currentX];

						uint8_t red = LCD_GetRed(color);
						uint8_t green = LCD_GetGreen(color);
						uint8_t blue = LCD_GetBlue(color);

						redTotal += red;
						greenTotal += green;
						blueTotal += blue;
					}
				}
			}
			uint8_t red = redTotal/9;
			uint8_t green = greenTotal/9;
			uint8_t blue = blueTotal/9;

			uint16_t color = (red << 11) | (green << 5) | blue;

			Buffer_DrawPx(x-xref, y-yref, color);
		}
	}
	free(temp);
}
void Buffer_Opacity(uint16_t opp){
	uint16_t i;
	uint16_t w = lcd_buffer.width;
	uint16_t h = lcd_buffer.height;
	uint16_t *ptr = lcd_buffer.p;
	for(i=0; i<w*h; i++){
		uint16_t col;

		col = LCD_GetRed(*ptr);
		if (col >= opp) *ptr -= opp << 11;
		else if (col > 0) *ptr &= 0x07FF;

		col = LCD_GetGreen(*ptr);
		if (col >= opp<<1) *ptr -= opp << 6;
		else if (col > 0) *ptr &= 0xF81F;

		col = LCD_GetBlue(*ptr);
		if (col >= opp)	*ptr -= opp;
		else if (col > 0) *ptr &= 0xFF80;

		ptr++;
	}
}
void Buffer_WriteToLCD(uint16_t x_pos, uint16_t y_pos){
	uint16_t w = lcd_buffer.width-1;
	uint16_t h = lcd_buffer.height-1;
	int16_t xref = lcd_buffer.xref;
	int16_t yref = lcd_buffer.yref;

	uint8_t *ptr_8 = lcd_buffer.p;
	uint16_t *ptr_16 = lcd_buffer.p;
	uint16_t x = x_pos - xref;
	uint16_t y = y_pos - yref;

	if ((x + w) >= LCD_WIDTH || (x + w) < 0 || (y + h) >= LCD_HEIGHT || (y + h) < 0 ){
		return;
	}
	uint16_t i;
	for (i = 0; i <= (w + 1)*(h + 1); i++){	//check
		ptr_16[i] = __REV16(ptr_16[i]);
	}
	LCD_SetAddressWindow(x, y, x + w, y + h);		//w&h - 0-239
#ifdef USE_DMA
	setDMAMemMode(mem_increase, mode_16bit);                                                            // Set SPI and DMA to 16 bit, enable memory increase
#else
	setSPI_Size(mode_16bit);                                                                            // Set SPI to 16 bit
#endif
	LCD_WriteData(ptr_8, (w+1)*(h+1));	//if 0, then null - min 1, max NA -> min 1*1
#ifdef USE_DMA
	setDMAMemMode(mem_increase, mode_8bit);                                                            // Set SPI and DMA to 16 bit, enable memory increase
#else
	setSPI_Size(mode_8bit);                                                                            // Set SPI to 16 bit
#endif
	ptr_16 = lcd_buffer.p;
	for (i = 0; i <= (w + 1)*(h + 1); i++){	//check
		*ptr_16 = __REV16(*ptr_16);
		ptr_16++;
	}
}
void Buffer_ReadFromLCD(uint16_t x, uint16_t y, uint16_t height, uint16_t width, int16_t xref, int16_t yref){
	if(x + width >= LCD_WIDTH || y + height >= LCD_HEIGHT){
		return;	//wrong params
	}
	if (lcd_buffer.p != NULL){
		free(lcd_buffer.p);
	}

	Buffer_Resize(width, height, xref, yref);

	LCD_SetAddressWindow(x-xref, y-yref, x-xref + width-1, y-yref + height-1);
	setSPI_Mode(MODE_READ);
	//skip first data bits
	uint8_t i,j,m;
	uint32_t temp;
	for (i=0; i<16; i++){
		HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_10);
	}
	//read data
	uint16_t *ptr = lcd_buffer.p;
	for (i = 0; i <= width; i++){
		for (j = 0; j <= height; j++){
			for(m = 0; m < 24; m++){
				temp = (temp << 1) + HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_12);
				HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_10);
				HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_10);
			}
			*ptr = ((temp & 0x00F80000) >> 8) | ((temp & 0x0000FC00) >> 5) | ((temp & 0x000000F8) >> 3);
			ptr++;
		}
	}
	setSPI_Mode(MODE_WRITE);
return;
}
void Buffer_Clear(){
	free(lcd_buffer.p);
	lcd_buffer.height = 0;
	lcd_buffer.width = 0;
	lcd_buffer.xref = 0;
	lcd_buffer.yref = 0;
}

void Buffer_DrawBMP(int16_t xref, int16_t yref, const UG_BMP* bmp){
	uint16_t w = bmp->width;
	uint16_t h = bmp->height;
	if ((xref > LCD_WIDTH-1) || (yref > LCD_HEIGHT-1))
		return;
	if ((xref + w - 1) > LCD_WIDTH-1)
		return;
	if ((yref + h - 1) > LCD_HEIGHT-1)
		return;
	if(bmp->bpp!=BMP_BPP_16)
		return;
	uint16_t newHeight = bmp->width > lcd_buffer.width ? bmp->width : lcd_buffer.width;
	uint16_t newWidth = bmp->height > lcd_buffer.height ? bmp->height : lcd_buffer.height;
	int16_t newxref = -xref > lcd_buffer.xref ? -xref : lcd_buffer.xref;
	int16_t newyref = -yref > lcd_buffer.yref ? -yref : lcd_buffer.yref;
	Buffer_Resize(newWidth, newHeight, newxref, newyref);
	uint16_t x,y;
	const uint16_t *ptrbmp = bmp->p;
	uint16_t *buff = lcd_buffer.p;
	for (y=0; y<h; y++){
		for(x=0; x<w; x++){
			uint16_t px = ptrbmp[y*h + x];
			if (px != 0x0000){
				uint16_t id = (y + newyref + yref) * newHeight + x + newxref + xref;
				buff[id] = __REV16(px);
			}
		}
	}
}

void Buffer_ColorFilter(uint16_t filter){
	uint16_t *ptr = lcd_buffer.p;
	uint16_t i;
	uint16_t size = lcd_buffer.height * lcd_buffer.width;
	for(i=0; i<size; i++){
		ptr[i] = ptr[i] & filter;
	}
	return;
}
