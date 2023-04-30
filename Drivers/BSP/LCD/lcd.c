//
// Created by Shin on 2023/4/30.
//
#if 1

#include "main.h"
#include "lcd.h"
#include "lcdfont.h"
#include "usart.h"


/* lcd_ex.c��Ÿ���LCD����IC�ļĴ�����ʼ�����ִ���,�Լ�lcd.c,��.c�ļ�
 * ��ֱ�Ӽ��뵽��������,ֻ��lcd.c���õ�,����ͨ��include����ʽ���.(��Ҫ��
 * �����ļ��ٰ�����.c�ļ�!!����ᱨ��!)
 */
#include "lcd_ex.h"

SRAM_HandleTypeDef g_sram_handle;    /* SRAM���(���ڿ���LCD) */

/* LCD�Ļ�����ɫ�ͱ���ɫ */
uint32_t g_point_color = 0XF800;    /* ������ɫ */
uint32_t g_back_color  = 0XFFFF;    /* ����ɫ */

/* ����LCD��Ҫ���� */
_lcd_dev lcddev;

/**
 * @brief       LCDд����
 * @param       data: Ҫд�������
 * @retval      ��
 */
void lcd_wr_data(volatile uint16_t data)
{
  data = data;            /* ʹ��-O2�Ż���ʱ��,����������ʱ */
  LCD->LCD_RAM = data;
}

/**
 * @brief       LCDд�Ĵ������/��ַ����
 * @param       regno: �Ĵ������/��ַ
 * @retval      ��
 */
void lcd_wr_regno(volatile uint16_t regno)
{
  regno = regno;          /* ʹ��-O2�Ż���ʱ��,����������ʱ */
  LCD->LCD_REG = regno;   /* д��Ҫд�ļĴ������ */
}

/**
 * @brief       LCDд�Ĵ���
 * @param       regno:�Ĵ������/��ַ
 * @param       data:Ҫд�������
 * @retval      ��
 */
void lcd_write_reg(uint16_t regno, uint16_t data)
{
  LCD->LCD_REG = regno;   /* д��Ҫд�ļĴ������ */
  LCD->LCD_RAM = data;    /* д������ */
}

/**
 * @brief       LCD��ʱ����,�����ڲ�����mdk -O1ʱ���Ż�ʱ��Ҫ���õĵط�
 * @param       t:��ʱ����ֵ
 * @retval      ��
 */
static void lcd_opt_delay(uint32_t i)
{
  while (i--);    /* ʹ��AC6ʱ��ѭ�����ܱ��Ż�,��ʹ��while(1) __asm volatile(""); */
}

/**
 * @brief       LCD������
 * @param       ��
 * @retval      ��ȡ��������
 */
static uint16_t lcd_rd_data(void)
{
  volatile uint16_t ram;  /* ��ֹ���Ż� */
  lcd_opt_delay(2);
  ram = LCD->LCD_RAM;
  return ram;
}



/**
 * @brief       ׼��дGRAM
 * @param       ��
 * @retval      ��
 */
void lcd_write_ram_prepare(void)
{
  LCD->LCD_REG = lcddev.wramcmd;
}

/**
 * @brief       ��ȡ��ĳ�����ɫֵ
 * @param       x,y:����
 * @retval      �˵����ɫ(32λ��ɫ,�������LTDC)
 */
uint32_t lcd_read_point(uint16_t x, uint16_t y)
{
  uint16_t r = 0, g = 0, b = 0;

  if (x >= lcddev.width || y >= lcddev.height)return 0;   /* �����˷�Χ,ֱ�ӷ��� */

  lcd_set_cursor(x, y);       /* �������� */

  if (lcddev.id == 0X5510)
  {
    lcd_wr_regno(0X2E00);   /* 5510 ���Ͷ�GRAMָ�� */
  }
  else
  {
    lcd_wr_regno(0X2E);     /* 9341/5310/1963/7789 �ȷ��Ͷ�GRAMָ�� */
  }


  r = lcd_rd_data();          /* �ٶ�(dummy read) */

  if (lcddev.id == 0X1963)return r;   /* 1963ֱ�Ӷ��Ϳ��� */

  r = lcd_rd_data();          /* ʵ��������ɫ */
  /* 9341/5310/5510/7789Ҫ��2�ζ��� */

  b = lcd_rd_data();
  g = r & 0XFF;       /* ����9341/5310/5510,��һ�ζ�ȡ����RG��ֵ,R��ǰ,G�ں�,��ռ8λ */
  g <<= 8;
  return (((r >> 11) << 11) | ((g >> 10) << 5) | (b >> 11));  /* ILI9341/NT35310/NT35510��Ҫ��ʽת��һ�� */
}

/**
 * @brief       LCD������ʾ
 * @param       ��
 * @retval      ��
 */
void lcd_display_on(void)
{
  if (lcddev.id == 0X5510)
  {
    lcd_wr_regno(0X2900);   /* ������ʾ */
  }
  else    /* 9341/5310/1963/7789 �ȷ��Ϳ�����ʾָ�� */
  {
    lcd_wr_regno(0X29);     /* ������ʾ */
  }

}

/**
 * @brief       LCD�ر���ʾ
 * @param       ��
 * @retval      ��
 */
void lcd_display_off(void)
{
  if (lcddev.id == 0X5510)
  {
    lcd_wr_regno(0X2800);   /* �ر���ʾ */
  }
  else    /* 9341/5310/1963/7789 �ȷ��͹ر���ʾָ�� */
  {
    lcd_wr_regno(0X28);     /* �ر���ʾ */
  }

}

/**
 * @brief       ���ù��λ��(��RGB����Ч)
 * @param       x,y: ����
 * @retval      ��
 */
void lcd_set_cursor(uint16_t x, uint16_t y)
{
  if (lcddev.id == 0X1963)
  {
    if (lcddev.dir == 0)    /* ����ģʽ, x������Ҫ�任 */
    {
      x = lcddev.width - 1 - x;
      lcd_wr_regno(lcddev.setxcmd);
      lcd_wr_data(0);
      lcd_wr_data(0);
      lcd_wr_data(x >> 8);
      lcd_wr_data(x & 0XFF);
    }
    else                    /* ����ģʽ */
    {
      lcd_wr_regno(lcddev.setxcmd);
      lcd_wr_data(x >> 8);
      lcd_wr_data(x & 0XFF);
      lcd_wr_data((lcddev.width - 1) >> 8);
      lcd_wr_data((lcddev.width - 1) & 0XFF);
    }

    lcd_wr_regno(lcddev.setycmd);
    lcd_wr_data(y >> 8);
    lcd_wr_data(y & 0XFF);
    lcd_wr_data((lcddev.height - 1) >> 8);
    lcd_wr_data((lcddev.height - 1) & 0XFF);

  }
  else if (lcddev.id == 0X5510)
  {
    lcd_wr_regno(lcddev.setxcmd);
    lcd_wr_data(x >> 8);
    lcd_wr_regno(lcddev.setxcmd + 1);
    lcd_wr_data(x & 0XFF);
    lcd_wr_regno(lcddev.setycmd);
    lcd_wr_data(y >> 8);
    lcd_wr_regno(lcddev.setycmd + 1);
    lcd_wr_data(y & 0XFF);
  }
  else    /* 9341/5310/7789 �� �������� */
  {
    lcd_wr_regno(lcddev.setxcmd);
    lcd_wr_data(x >> 8);
    lcd_wr_data(x & 0XFF);
    lcd_wr_regno(lcddev.setycmd);
    lcd_wr_data(y >> 8);
    lcd_wr_data(y & 0XFF);
  }
}

/**
 * @brief       ����LCD���Զ�ɨ�跽��(��RGB����Ч)
 *   @note
 *              9341/5310/5510/1963/7789��IC�Ѿ�ʵ�ʲ���
 *              ע��:�����������ܻ��ܵ��˺������õ�Ӱ��(������9341),
 *              ����,һ������ΪL2R_U2D����,�������Ϊ����ɨ�跽ʽ,���ܵ�����ʾ������.
 *
 * @param       dir:0~7,����8������(���嶨���lcd.h)
 * @retval      ��
 */
void lcd_scan_dir(uint8_t dir)
{
  uint16_t regval = 0;
  uint16_t dirreg = 0;
  uint16_t temp;

  /* ����ʱ����1963���ı�ɨ�跽������IC�ı�ɨ�跽������ʱ1963�ı䷽������IC���ı�ɨ�跽�� */
  if ((lcddev.dir == 1 && lcddev.id != 0X1963) || (lcddev.dir == 0 && lcddev.id == 0X1963))
  {
    switch (dir)   /* ����ת�� */
    {
      case 0:
        dir = 6;
        break;

      case 1:
        dir = 7;
        break;

      case 2:
        dir = 4;
        break;

      case 3:
        dir = 5;
        break;

      case 4:
        dir = 1;
        break;

      case 5:
        dir = 0;
        break;

      case 6:
        dir = 3;
        break;

      case 7:
        dir = 2;
        break;
    }
  }


  /* ����ɨ�跽ʽ ���� 0X36/0X3600 �Ĵ��� bit 5,6,7 λ��ֵ */
  switch (dir)
  {
    case L2R_U2D:/* ������,���ϵ��� */
      regval |= (0 << 7) | (0 << 6) | (0 << 5);
      break;

    case L2R_D2U:/* ������,���µ��� */
      regval |= (1 << 7) | (0 << 6) | (0 << 5);
      break;

    case R2L_U2D:/* ���ҵ���,���ϵ��� */
      regval |= (0 << 7) | (1 << 6) | (0 << 5);
      break;

    case R2L_D2U:/* ���ҵ���,���µ��� */
      regval |= (1 << 7) | (1 << 6) | (0 << 5);
      break;

    case U2D_L2R:/* ���ϵ���,������ */
      regval |= (0 << 7) | (0 << 6) | (1 << 5);
      break;

    case U2D_R2L:/* ���ϵ���,���ҵ��� */
      regval |= (0 << 7) | (1 << 6) | (1 << 5);
      break;

    case D2U_L2R:/* ���µ���,������ */
      regval |= (1 << 7) | (0 << 6) | (1 << 5);
      break;

    case D2U_R2L:/* ���µ���,���ҵ��� */
      regval |= (1 << 7) | (1 << 6) | (1 << 5);
      break;
  }

  dirreg = 0X36;  /* �Ծ��󲿷�����IC, ��0X36�Ĵ������� */

  if (lcddev.id == 0X5510)
  {
    dirreg = 0X3600;    /* ����5510, ����������ic�ļĴ����в��� */
  }

  /* 9341 & 7789 Ҫ����BGRλ */
  if (lcddev.id == 0X9341 || lcddev.id == 0X7789)
  {
    regval |= 0X08;
  }

  lcd_write_reg(dirreg, regval);

  if (lcddev.id != 0X1963)   /* 1963�������괦�� */
  {
    if (regval & 0X20)
    {
      if (lcddev.width < lcddev.height)   /* ����X,Y */
      {
        temp = lcddev.width;
        lcddev.width = lcddev.height;
        lcddev.height = temp;
      }
    }
    else
    {
      if (lcddev.width > lcddev.height)   /* ����X,Y */
      {
        temp = lcddev.width;
        lcddev.width = lcddev.height;
        lcddev.height = temp;
      }
    }
  }

  /* ������ʾ����(����)��С */
  if (lcddev.id == 0X5510)
  {
    lcd_wr_regno(lcddev.setxcmd);
    lcd_wr_data(0);
    lcd_wr_regno(lcddev.setxcmd + 1);
    lcd_wr_data(0);
    lcd_wr_regno(lcddev.setxcmd + 2);
    lcd_wr_data((lcddev.width - 1) >> 8);
    lcd_wr_regno(lcddev.setxcmd + 3);
    lcd_wr_data((lcddev.width - 1) & 0XFF);
    lcd_wr_regno(lcddev.setycmd);
    lcd_wr_data(0);
    lcd_wr_regno(lcddev.setycmd + 1);
    lcd_wr_data(0);
    lcd_wr_regno(lcddev.setycmd + 2);
    lcd_wr_data((lcddev.height - 1) >> 8);
    lcd_wr_regno(lcddev.setycmd + 3);
    lcd_wr_data((lcddev.height - 1) & 0XFF);
  }
  else
  {
    lcd_wr_regno(lcddev.setxcmd);
    lcd_wr_data(0);
    lcd_wr_data(0);
    lcd_wr_data((lcddev.width - 1) >> 8);
    lcd_wr_data((lcddev.width - 1) & 0XFF);
    lcd_wr_regno(lcddev.setycmd);
    lcd_wr_data(0);
    lcd_wr_data(0);
    lcd_wr_data((lcddev.height - 1) >> 8);
    lcd_wr_data((lcddev.height - 1) & 0XFF);
  }
}

/**
 * @brief       ����
 * @param       x,y: ����
 * @param       color: �����ɫ(32λ��ɫ,�������LTDC)
 * @retval      ��
 */
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color)
{
  lcd_set_cursor(x, y);       /* ���ù��λ�� */
  lcd_write_ram_prepare();    /* ��ʼд��GRAM */
  LCD->LCD_RAM = color;
}

/**
 * @brief       SSD1963�����������ú���
 * @param       pwm: ����ȼ�,0~100.Խ��Խ��.
 * @retval      ��
 */
void lcd_ssd_backlight_set(uint8_t pwm)
{
  lcd_wr_regno(0xBE);         /* ����PWM��� */
  lcd_wr_data(0x05);          /* 1����PWMƵ�� */
  lcd_wr_data(pwm * 2.55);    /* 2����PWMռ�ձ� */
  lcd_wr_data(0x01);          /* 3����C */
  lcd_wr_data(0xFF);          /* 4����D */
  lcd_wr_data(0x00);          /* 5����E */
  lcd_wr_data(0x00);          /* 6����F */
}

/**
 * @brief       ����LCD��ʾ����
 * @param       dir:0,����; 1,����
 * @retval      ��
 */
void lcd_display_dir(uint8_t dir)
{
  lcddev.dir = dir;   /* ����/���� */

  if (dir == 0)       /* ���� */
  {
    lcddev.width = 240;
    lcddev.height = 320;

    if (lcddev.id == 0x5510)
    {
      lcddev.wramcmd = 0X2C00;
      lcddev.setxcmd = 0X2A00;
      lcddev.setycmd = 0X2B00;
      lcddev.width = 480;
      lcddev.height = 800;
    }
    else if (lcddev.id == 0X1963)
    {
      lcddev.wramcmd = 0X2C;  /* ����д��GRAM��ָ�� */
      lcddev.setxcmd = 0X2B;  /* ����дX����ָ�� */
      lcddev.setycmd = 0X2A;  /* ����дY����ָ�� */
      lcddev.width = 480;     /* ���ÿ��480 */
      lcddev.height = 800;    /* ���ø߶�800 */
    }
    else   /* ����IC, ����: 9341 / 5310 / 7789��IC */
    {
      lcddev.wramcmd = 0X2C;
      lcddev.setxcmd = 0X2A;
      lcddev.setycmd = 0X2B;
    }

    if (lcddev.id == 0X5310)    /* �����5310 ���ʾ�� 320*480�ֱ��� */
    {
      lcddev.width = 320;
      lcddev.height = 480;
    }
  }       /* dir = 0 */
  else                /* ���� */
  {
    lcddev.width = 320;         /* Ĭ�Ͽ�� */
    lcddev.height = 240;        /* Ĭ�ϸ߶� */

    if (lcddev.id == 0x5510)
    {
      lcddev.wramcmd = 0X2C00;
      lcddev.setxcmd = 0X2A00;
      lcddev.setycmd = 0X2B00;
      lcddev.width = 800;
      lcddev.height = 480;
    }
    else if (lcddev.id == 0X1963)
    {
      lcddev.wramcmd = 0X2C;  /* ����д��GRAM��ָ�� */
      lcddev.setxcmd = 0X2A;  /* ����дX����ָ�� */
      lcddev.setycmd = 0X2B;  /* ����дY����ָ�� */
      lcddev.width = 800;     /* ���ÿ��800 */
      lcddev.height = 480;    /* ���ø߶�480 */
    }
    else   /* ����IC, ����: 9341 / 5310 / 7789��IC */
    {
      lcddev.wramcmd = 0X2C;
      lcddev.setxcmd = 0X2A;
      lcddev.setycmd = 0X2B;
    }

    if (lcddev.id == 0X5310)   /* �����5310 ���ʾ�� 320*480�ֱ��� */
    {
      lcddev.width = 480;
      lcddev.height = 320;
    }
  }

  lcd_scan_dir(DFT_SCAN_DIR);     /* Ĭ��ɨ�跽�� */
}

/**
 * @brief       ���ô���(��RGB����Ч),���Զ����û������굽�������Ͻ�(sx,sy).
 * @param       sx,sy:������ʼ����(���Ͻ�)
 * @param       width,height:���ڿ�Ⱥ͸߶�,�������0!!
 *   @note      �����С:width*height.
 *
 * @retval      ��
 */
void lcd_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{
  uint16_t twidth, theight;
  twidth = sx + width - 1;
  theight = sy + height - 1;


  if (lcddev.id == 0X1963 && lcddev.dir != 1)     /* 1963�������⴦�� */
  {
    sx = lcddev.width - width - sx;
    height = sy + height - 1;
    lcd_wr_regno(lcddev.setxcmd);
    lcd_wr_data(sx >> 8);
    lcd_wr_data(sx & 0XFF);
    lcd_wr_data((sx + width - 1) >> 8);
    lcd_wr_data((sx + width - 1) & 0XFF);
    lcd_wr_regno(lcddev.setycmd);
    lcd_wr_data(sy >> 8);
    lcd_wr_data(sy & 0XFF);
    lcd_wr_data(height >> 8);
    lcd_wr_data(height & 0XFF);
  }
  else if (lcddev.id == 0X5510)
  {
    lcd_wr_regno(lcddev.setxcmd);
    lcd_wr_data(sx >> 8);
    lcd_wr_regno(lcddev.setxcmd + 1);
    lcd_wr_data(sx & 0XFF);
    lcd_wr_regno(lcddev.setxcmd + 2);
    lcd_wr_data(twidth >> 8);
    lcd_wr_regno(lcddev.setxcmd + 3);
    lcd_wr_data(twidth & 0XFF);
    lcd_wr_regno(lcddev.setycmd);
    lcd_wr_data(sy >> 8);
    lcd_wr_regno(lcddev.setycmd + 1);
    lcd_wr_data(sy & 0XFF);
    lcd_wr_regno(lcddev.setycmd + 2);
    lcd_wr_data(theight >> 8);
    lcd_wr_regno(lcddev.setycmd + 3);
    lcd_wr_data(theight & 0XFF);
  }
  else    /* 9341/5310/7789/1963���� �� ���ô��� */
  {
    lcd_wr_regno(lcddev.setxcmd);
    lcd_wr_data(sx >> 8);
    lcd_wr_data(sx & 0XFF);
    lcd_wr_data(twidth >> 8);
    lcd_wr_data(twidth & 0XFF);
    lcd_wr_regno(lcddev.setycmd);
    lcd_wr_data(sy >> 8);
    lcd_wr_data(sy & 0XFF);
    lcd_wr_data(theight >> 8);
    lcd_wr_data(theight & 0XFF);
  }
}

/**
 * @brief       SRAM�ײ�������ʱ��ʹ�ܣ����ŷ���
 * @note        �˺����ᱻHAL_SRAM_Init()����,��ʼ����д��������
 * @param       hsram:SRAM���
 * @retval      ��
 */
void myHAL_SRAM_MspInit(SRAM_HandleTypeDef *hsram)
{
  GPIO_InitTypeDef gpio_init_struct;

  __HAL_RCC_FSMC_CLK_ENABLE();            /* ʹ��FSMCʱ�� */
  __HAL_RCC_GPIOD_CLK_ENABLE();           /* ʹ��GPIODʱ�� */
  __HAL_RCC_GPIOE_CLK_ENABLE();           /* ʹ��GPIOEʱ�� */

  /* ��ʼ��PD0,1,8,9,10,14,15 */
  gpio_init_struct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 \
                           | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
  gpio_init_struct.Mode = GPIO_MODE_AF_PP;                  /* ���츴�� */
  gpio_init_struct.Pull = GPIO_PULLUP;                      /* ���� */
  gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;            /* ���� */
  HAL_GPIO_Init(GPIOD, &gpio_init_struct);                  /* ��ʼ�� */

  /* ��ʼ��PE7,8,9,10,11,12,13,14,15 */
  gpio_init_struct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 \
                           | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOE, &gpio_init_struct);
}

/**
 * @brief       ��ʼ��LCD
 *   @note      �ó�ʼ���������Գ�ʼ�������ͺŵ�LCD(�����.c�ļ���ǰ�������)
 *
 * @param       ��
 * @retval      ��
 */
void lcd_init(void)
{
  GPIO_InitTypeDef gpio_init_struct;
  FSMC_NORSRAM_TimingTypeDef fsmc_read_handle;
  FSMC_NORSRAM_TimingTypeDef fsmc_write_handle;

  LCD_CS_GPIO_CLK_ENABLE();   /* LCD_CS��ʱ��ʹ�� */
  LCD_WR_GPIO_CLK_ENABLE();   /* LCD_WR��ʱ��ʹ�� */
  LCD_RD_GPIO_CLK_ENABLE();   /* LCD_RD��ʱ��ʹ�� */
  LCD_RS_GPIO_CLK_ENABLE();   /* LCD_RS��ʱ��ʹ�� */
  LCD_BL_GPIO_CLK_ENABLE();   /* LCD_BL��ʱ��ʹ�� */

  gpio_init_struct.Pin = LCD_CS_GPIO_PIN;
  gpio_init_struct.Mode = GPIO_MODE_AF_PP;                /* ���츴�� */
  gpio_init_struct.Pull = GPIO_PULLUP;                    /* ���� */
  gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* ���� */
  HAL_GPIO_Init(LCD_CS_GPIO_PORT, &gpio_init_struct);     /* ��ʼ��LCD_CS���� */

  gpio_init_struct.Pin = LCD_WR_GPIO_PIN;
  HAL_GPIO_Init(LCD_WR_GPIO_PORT, &gpio_init_struct);     /* ��ʼ��LCD_WR���� */

  gpio_init_struct.Pin = LCD_RD_GPIO_PIN;
  HAL_GPIO_Init(LCD_RD_GPIO_PORT, &gpio_init_struct);     /* ��ʼ��LCD_RD���� */

  gpio_init_struct.Pin = LCD_RS_GPIO_PIN;
  HAL_GPIO_Init(LCD_RS_GPIO_PORT, &gpio_init_struct);     /* ��ʼ��LCD_RS���� */

  gpio_init_struct.Pin = LCD_BL_GPIO_PIN;
  gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;            /* ������� */
  HAL_GPIO_Init(LCD_BL_GPIO_PORT, &gpio_init_struct);     /* LCD_BL����ģʽ����(�������) */

  g_sram_handle.Instance = FSMC_NORSRAM_DEVICE;
  g_sram_handle.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;

  g_sram_handle.Init.NSBank = FSMC_NORSRAM_BANK4;                        /* ʹ��NE4 */
  g_sram_handle.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;     /* ��ַ/�����߲����� */
  g_sram_handle.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;    /* 16λ���ݿ�� */
  g_sram_handle.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;   /* �Ƿ�ʹ��ͻ������,����ͬ��ͻ���洢����Ч,�˴�δ�õ� */
  g_sram_handle.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW; /* �ȴ��źŵļ���,����ͻ��ģʽ���������� */
  g_sram_handle.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;      /* �洢�����ڵȴ�����֮ǰ��һ��ʱ�����ڻ��ǵȴ������ڼ�ʹ��NWAIT */
  g_sram_handle.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;       /* �洢��дʹ�� */
  g_sram_handle.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;              /* �ȴ�ʹ��λ,�˴�δ�õ� */
  g_sram_handle.Init.ExtendedMode = FSMC_EXTENDED_MODE_ENABLE;           /* ��дʹ�ò�ͬ��ʱ�� */
  g_sram_handle.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;  /* �Ƿ�ʹ��ͬ������ģʽ�µĵȴ��ź�,�˴�δ�õ� */
  g_sram_handle.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;              /* ��ֹͻ��д */


  /* FSMC��ʱ����ƼĴ��� */
  fsmc_read_handle.AddressSetupTime = 0;      /* ��ַ����ʱ��(ADDSET)Ϊ1��HCLK 1/72M = 13.9ns (ʵ�� > 200ns) */
  fsmc_read_handle.AddressHoldTime = 0;       /* ��ַ����ʱ��(ADDHLD) ģʽA��û���õ� */
  /* ��ΪҺ������IC�Ķ����ݵ�ʱ���ٶȲ���̫��,�����Ǹ�������оƬ */
  fsmc_read_handle.DataSetupTime = 15;        /* ���ݱ���ʱ��(DATAST)Ϊ16��HCLK = 13.9 * 16 = 222.4ns */
  fsmc_read_handle.AccessMode = FSMC_ACCESS_MODE_A;    /* ģʽA */

  /* FSMCдʱ����ƼĴ��� */
  fsmc_write_handle.AddressSetupTime = 0;     /* ��ַ����ʱ��(ADDSET)Ϊ1��HCLK = 13.9ns */
  fsmc_write_handle.AddressHoldTime = 0;      /* ��ַ����ʱ��(ADDHLD) ģʽA��û���õ� */
  /* ĳЩҺ������IC��д�ź���������Ҳ��50ns�� */
  fsmc_write_handle.DataSetupTime = 1;        /* ���ݱ���ʱ��(DATAST)Ϊ2��HCLK = 13.9 * 2 = 27.8ns (ʵ�� > 200ns) */
  fsmc_write_handle.AccessMode = FSMC_ACCESS_MODE_A;   /* ģʽA */

  HAL_SRAM_Init(&g_sram_handle, &fsmc_read_handle, &fsmc_write_handle);
//  delay_ms(50);   /* ��ʼ��FSMC��,����ȴ�һ��ʱ����ܿ�ʼ��ʼ�� */
  HAL_Delay(50);   /* ��ʼ��FSMC��,����ȴ�һ��ʱ����ܿ�ʼ��ʼ�� */

  /* ����9341 ID�Ķ�ȡ */
  lcd_wr_regno(0XD3);
  lcddev.id = lcd_rd_data();  /* dummy read */
  lcddev.id = lcd_rd_data();  /* ����0X00 */
  lcddev.id = lcd_rd_data();  /* ��ȡ93 */
  lcddev.id <<= 8;
  lcddev.id |= lcd_rd_data(); /* ��ȡ41 */

  if (lcddev.id != 0X9341)    /* ���� 9341 , ���Կ����ǲ��� ST7789 */
  {
    lcd_wr_regno(0X04);
    lcddev.id = lcd_rd_data();      /* dummy read */
    lcddev.id = lcd_rd_data();      /* ����0X85 */
    lcddev.id = lcd_rd_data();      /* ��ȡ0X85 */
    lcddev.id <<= 8;
    lcddev.id |= lcd_rd_data();     /* ��ȡ0X52 */

    if (lcddev.id == 0X8552)        /* ��8552��IDת����7789 */
    {
      lcddev.id = 0x7789;
    }

    if (lcddev.id != 0x7789)        /* Ҳ����ST7789, �����ǲ��� NT35310 */
    {
      lcd_wr_regno(0XD4);
      lcddev.id = lcd_rd_data();  /* dummy read */
      lcddev.id = lcd_rd_data();  /* ����0X01 */
      lcddev.id = lcd_rd_data();  /* ����0X53 */
      lcddev.id <<= 8;
      lcddev.id |= lcd_rd_data(); /* �������0X10 */

      if (lcddev.id != 0X5310)    /* Ҳ����NT35310,���Կ����ǲ���NT35510 */
      {
        /* ������Կ�������ṩ,�հἴ�ɣ� */
        lcd_write_reg(0xF000, 0x0055);
        lcd_write_reg(0xF001, 0x00AA);
        lcd_write_reg(0xF002, 0x0052);
        lcd_write_reg(0xF003, 0x0008);
        lcd_write_reg(0xF004, 0x0001);

        lcd_wr_regno(0xC500);           /* ��ȡID��8λ */
        lcddev.id = lcd_rd_data();      /* ����0X55 */
        lcddev.id <<= 8;

        lcd_wr_regno(0xC501);           /* ��ȡID��8λ */
        lcddev.id |= lcd_rd_data();     /* ����0X10 */
//        delay_ms(5);
        HAL_Delay(5);

        if (lcddev.id != 0X5510)        /* Ҳ����NT5510,���Կ����ǲ���SSD1963 */
        {
          lcd_wr_regno(0XA1);
          lcddev.id = lcd_rd_data();
          lcddev.id = lcd_rd_data();  /* ����0X57 */
          lcddev.id <<= 8;
          lcddev.id |= lcd_rd_data(); /* ����0X61 */

          if (lcddev.id == 0X5761)lcddev.id = 0X1963; /* SSD1963���ص�ID��5761H,Ϊ��������,����ǿ������Ϊ1963 */
        }
      }
    }
  }

  /* �ر�ע��, �����main�����������δ���1��ʼ��, ��Ῠ����printf
   * ����(������f_putc����), ����, �����ʼ������1, �������ε�����
   * ���� printf ��� !!!!!!!
   */
//  printf("LCD ID:%x\r\n", lcddev.id); /* ��ӡLCD ID */
  print_UARTx(huart1, "LCD ID:%x\r\n", lcddev.id); /* ��ӡLCD ID */

  if (lcddev.id == 0X7789)
  {
    lcd_ex_st7789_reginit();    /* ִ��ST7789��ʼ�� */
  }
  else if (lcddev.id == 0X9341)
  {
    lcd_ex_ili9341_reginit();   /* ִ��ILI9341��ʼ�� */
  }
  else if (lcddev.id == 0x5310)
  {
    lcd_ex_nt35310_reginit();   /* ִ��NT35310��ʼ�� */
  }
  else if (lcddev.id == 0x5510)
  {
    lcd_ex_nt35510_reginit();   /* ִ��NT35510��ʼ�� */
  }
  else if (lcddev.id == 0X1963)
  {
    lcd_ex_ssd1963_reginit();   /* ִ��SSD1963��ʼ�� */
    lcd_ssd_backlight_set(100); /* ��������Ϊ���� */
  }

  lcd_display_dir(0); /* Ĭ��Ϊ���� */
  LCD_BL(1);          /* �������� */
  lcd_clear(WHITE);
}

/**
 * @brief       ��������
 * @param       color: Ҫ��������ɫ
 * @retval      ��
 */
void lcd_clear(uint16_t color)
{
  uint32_t index = 0;
  uint32_t totalpoint = lcddev.width;
  totalpoint *= lcddev.height;    /* �õ��ܵ��� */
  lcd_set_cursor(0x00, 0x0000);   /* ���ù��λ�� */
  lcd_write_ram_prepare();        /* ��ʼд��GRAM */

  for (index = 0; index < totalpoint; index++)
  {
    LCD->LCD_RAM = color;
  }
}

/**
 * @brief       ��ָ����������䵥����ɫ
 * @param       (sx,sy),(ex,ey):�����ζԽ�����,�����СΪ:(ex - sx + 1) * (ey - sy + 1)
 * @param       color:  Ҫ������ɫ(32λ��ɫ,�������LTDC)
 * @retval      ��
 */
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
  uint16_t i, j;
  uint16_t xlen = 0;
  xlen = ex - sx + 1;

  for (i = sy; i <= ey; i++)
  {
    lcd_set_cursor(sx, i);      /* ���ù��λ�� */
    lcd_write_ram_prepare();    /* ��ʼд��GRAM */

    for (j = 0; j < xlen; j++)
    {
      LCD->LCD_RAM = color;   /* ��ʾ��ɫ */
    }
  }
}

/**
 * @brief       ��ָ�����������ָ����ɫ��
 * @param       (sx,sy),(ex,ey):�����ζԽ�����,�����СΪ:(ex - sx + 1) * (ey - sy + 1)
 * @param       color: Ҫ������ɫ�����׵�ַ
 * @retval      ��
 */
void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color)
{
  uint16_t height, width;
  uint16_t i, j;
  width = ex - sx + 1;            /* �õ����Ŀ�� */
  height = ey - sy + 1;           /* �߶� */

  for (i = 0; i < height; i++)
  {
    lcd_set_cursor(sx, sy + i); /* ���ù��λ�� */
    lcd_write_ram_prepare();    /* ��ʼд��GRAM */

    for (j = 0; j < width; j++)
    {
      LCD->LCD_RAM = color[i * width + j]; /* д������ */
    }
  }
}

/**
 * @brief       ����
 * @param       x1,y1: �������
 * @param       x2,y2: �յ�����
 * @param       color: �ߵ���ɫ
 * @retval      ��
 */
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
  uint16_t t;
  int xerr = 0, yerr = 0, delta_x, delta_y, distance;
  int incx, incy, row, col;
  delta_x = x2 - x1;          /* ������������ */
  delta_y = y2 - y1;
  row = x1;
  col = y1;

  if (delta_x > 0)incx = 1;   /* ���õ������� */
  else if (delta_x == 0)incx = 0; /* ��ֱ�� */
  else
  {
    incx = -1;
    delta_x = -delta_x;
  }

  if (delta_y > 0)incy = 1;
  else if (delta_y == 0)incy = 0; /* ˮƽ�� */
  else
  {
    incy = -1;
    delta_y = -delta_y;
  }

  if ( delta_x > delta_y)distance = delta_x;  /* ѡȡ�������������� */
  else distance = delta_y;

  for (t = 0; t <= distance + 1; t++ )   /* ������� */
  {
    lcd_draw_point(row, col, color); /* ���� */
    xerr += delta_x ;
    yerr += delta_y ;

    if (xerr > distance)
    {
      xerr -= distance;
      row += incx;
    }

    if (yerr > distance)
    {
      yerr -= distance;
      col += incy;
    }
  }
}

/**
 * @brief       ��ˮƽ��
 * @param       x0,y0: �������
 * @param       len  : �߳���
 * @param       color: ���ε���ɫ
 * @retval      ��
 */
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color)
{
  if ((len == 0) || (x > lcddev.width) || (y > lcddev.height))return;

  lcd_fill(x, y, x + len - 1, y, color);
}

/**
 * @brief       ������
 * @param       x1,y1: �������
 * @param       x2,y2: �յ�����
 * @param       color: ���ε���ɫ
 * @retval      ��
 */
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
  lcd_draw_line(x1, y1, x2, y1, color);
  lcd_draw_line(x1, y1, x1, y2, color);
  lcd_draw_line(x1, y2, x2, y2, color);
  lcd_draw_line(x2, y1, x2, y2, color);
}

/**
 * @brief       ��Բ
 * @param       x,y  : Բ��������
 * @param       r    : �뾶
 * @param       color: Բ����ɫ
 * @retval      ��
 */
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
  int a, b;
  int di;
  a = 0;
  b = r;
  di = 3 - (r << 1);       /* �ж��¸���λ�õı�־ */

  while (a <= b)
  {
    lcd_draw_point(x0 + a, y0 - b, color);  /* 5 */
    lcd_draw_point(x0 + b, y0 - a, color);  /* 0 */
    lcd_draw_point(x0 + b, y0 + a, color);  /* 4 */
    lcd_draw_point(x0 + a, y0 + b, color);  /* 6 */
    lcd_draw_point(x0 - a, y0 + b, color);  /* 1 */
    lcd_draw_point(x0 - b, y0 + a, color);
    lcd_draw_point(x0 - a, y0 - b, color);  /* 2 */
    lcd_draw_point(x0 - b, y0 - a, color);  /* 7 */
    a++;

    /* ʹ��Bresenham�㷨��Բ */
    if (di < 0)
    {
      di += 4 * a + 6;
    }
    else
    {
      di += 10 + 4 * (a - b);
      b--;
    }
  }
}

/**
 * @brief       ���ʵ��Բ
 * @param       x0,y0: Բ��������
 * @param       r    : �뾶
 * @param       color: Բ����ɫ
 * @retval      ��
 */
void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color)
{
  uint32_t i;
  uint32_t imax = ((uint32_t)r * 707) / 1000 + 1;
  uint32_t sqmax = (uint32_t)r * (uint32_t)r + (uint32_t)r / 2;
  uint32_t xr = r;

  lcd_draw_hline(x - r, y, 2 * r, color);

  for (i = 1; i <= imax; i++)
  {
    if ((i * i + xr * xr) > sqmax)
    {
      /* draw lines from outside */
      if (xr > imax)
      {
        lcd_draw_hline (x - i + 1, y + xr, 2 * (i - 1), color);
        lcd_draw_hline (x - i + 1, y - xr, 2 * (i - 1), color);
      }

      xr--;
    }

    /* draw lines from inside (center) */
    lcd_draw_hline(x - xr, y + i, 2 * xr, color);
    lcd_draw_hline(x - xr, y - i, 2 * xr, color);
  }
}

/**
 * @brief       ��ָ��λ����ʾһ���ַ�
 * @param       x,y  : ����
 * @param       chr  : Ҫ��ʾ���ַ�:" "--->"~"
 * @param       size : �����С 12/16/24/32
 * @param       mode : ���ӷ�ʽ(1); �ǵ��ӷ�ʽ(0);
 * @retval      ��
 */
void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color)
{
  uint8_t temp, t1, t;
  uint16_t y0 = y;
  uint8_t csize = 0;
  uint8_t *pfont = 0;

  csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); /* �õ�����һ���ַ���Ӧ������ռ���ֽ��� */
  chr = chr - ' ';    /* �õ�ƫ�ƺ��ֵ��ASCII�ֿ��Ǵӿո�ʼȡģ������-' '���Ƕ�Ӧ�ַ����ֿ⣩ */

  switch (size)
  {
    case 12:
      pfont = (uint8_t *)asc2_1206[chr];  /* ����1206���� */
      break;

    case 16:
      pfont = (uint8_t *)asc2_1608[chr];  /* ����1608���� */
      break;

    case 24:
      pfont = (uint8_t *)asc2_2412[chr];  /* ����2412���� */
      break;

    case 32:
      pfont = (uint8_t *)asc2_3216[chr];  /* ����3216���� */
      break;

    default:
      return ;
  }

  for (t = 0; t < csize; t++)
  {
    temp = pfont[t];    /* ��ȡ�ַ��ĵ������� */

    for (t1 = 0; t1 < 8; t1++)   /* һ���ֽ�8���� */
    {
      if (temp & 0x80)        /* ��Ч��,��Ҫ��ʾ */
      {
        lcd_draw_point(x, y, color);        /* �������,Ҫ��ʾ����� */
      }
      else if (mode == 0)     /* ��Ч��,����ʾ */
      {
        lcd_draw_point(x, y, g_back_color); /* ������ɫ,�൱������㲻��ʾ(ע�ⱳ��ɫ��ȫ�ֱ�������) */
      }

      temp <<= 1; /* ��λ, �Ա��ȡ��һ��λ��״̬ */
      y++;

      if (y >= lcddev.height)return;  /* �������� */

      if ((y - y0) == size)   /* ��ʾ��һ����? */
      {
        y = y0; /* y���긴λ */
        x++;    /* x������� */

        if (x >= lcddev.width)return;   /* x���곬������ */

        break;
      }
    }
  }
}

/**
 * @brief       ƽ������, m^n
 * @param       m: ����
 * @param       n: ָ��
 * @retval      m��n�η�
 */
static uint32_t lcd_pow(uint8_t m, uint8_t n)
{
  uint32_t result = 1;

  while (n--)result *= m;

  return result;
}

/**
 * @brief       ��ʾlen������
 * @param       x,y : ��ʼ����
 * @param       num : ��ֵ(0 ~ 2^32)
 * @param       len : ��ʾ���ֵ�λ��
 * @param       size: ѡ������ 12/16/24/32
 * @retval      ��
 */
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color)
{
  uint8_t t, temp;
  uint8_t enshow = 0;

  for (t = 0; t < len; t++)   /* ������ʾλ��ѭ�� */
  {
    temp = (num / lcd_pow(10, len - t - 1)) % 10;   /* ��ȡ��Ӧλ������ */

    if (enshow == 0 && t < (len - 1))   /* û��ʹ����ʾ,�һ���λҪ��ʾ */
    {
      if (temp == 0)
      {
        lcd_show_char(x + (size / 2)*t, y, ' ', size, 0, color);/* ��ʾ�ո�,ռλ */
        continue;   /* �����¸�һλ */
      }
      else
      {
        enshow = 1; /* ʹ����ʾ */
      }

    }

    lcd_show_char(x + (size / 2)*t, y, temp + '0', size, 0, color); /* ��ʾ�ַ� */
  }
}

/**
 * @brief       ��չ��ʾlen������(��λ��0Ҳ��ʾ)
 * @param       x,y : ��ʼ����
 * @param       num : ��ֵ(0 ~ 2^32)
 * @param       len : ��ʾ���ֵ�λ��
 * @param       size: ѡ������ 12/16/24/32
 * @param       mode: ��ʾģʽ
 *              [7]:0,�����;1,���0.
 *              [6:1]:����
 *              [0]:0,�ǵ�����ʾ;1,������ʾ.
 *
 * @retval      ��
 */
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color)
{
  uint8_t t, temp;
  uint8_t enshow = 0;

  for (t = 0; t < len; t++)   /* ������ʾλ��ѭ�� */
  {
    temp = (num / lcd_pow(10, len - t - 1)) % 10;    /* ��ȡ��Ӧλ������ */

    if (enshow == 0 && t < (len - 1))   /* û��ʹ����ʾ,�һ���λҪ��ʾ */
    {
      if (temp == 0)
      {
        if (mode & 0X80)   /* ��λ��Ҫ���0 */
        {
          lcd_show_char(x + (size / 2)*t, y, '0', size, mode & 0X01, color);  /* ��0ռλ */
        }
        else
        {
          lcd_show_char(x + (size / 2)*t, y, ' ', size, mode & 0X01, color);  /* �ÿո�ռλ */
        }

        continue;
      }
      else
      {
        enshow = 1; /* ʹ����ʾ */
      }

    }

    lcd_show_char(x + (size / 2)*t, y, temp + '0', size, mode & 0X01, color);
  }
}

/**
 * @brief       ��ʾ�ַ���
 * @param       x,y         : ��ʼ����
 * @param       width,height: �����С
 * @param       size        : ѡ������ 12/16/24/32
 * @param       p           : �ַ����׵�ַ
 * @retval      ��
 */
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color)
{
  uint8_t x0 = x;
  width += x;
  height += y;

  while ((*p <= '~') && (*p >= ' '))   /* �ж��ǲ��ǷǷ��ַ�! */
  {
    if (x >= width)
    {
      x = x0;
      y += size;
    }

    if (y >= height)break;  /* �˳� */

    lcd_show_char(x, y, *p, size, 0, color);
    x += size / 2;
    p++;
  }
}

#else

#include "lcd.h"
#include "stdlib.h"
#include "font.h"
#include "usart.h"


SRAM_HandleTypeDef TFTSRAM_Handler;    //SRAM���(���ڿ���LCD)

//LCD�Ļ�����ɫ�ͱ���ɫ
uint32_t POINT_COLOR=0xFF000000;		//������ɫ
uint32_t BACK_COLOR =0xFFFFFFFF;  	//����ɫ

//����LCD��Ҫ����
//Ĭ��Ϊ����
_lcd_dev lcddev;

//д�Ĵ�������
//regval:�Ĵ���ֵ
void LCD_WR_REG(__IO uint16_t regval)
{
  regval=regval;		//ʹ��-O2�Ż���ʱ��,����������ʱ
  TFT_LCD->LCD_REG=regval;//д��Ҫд�ļĴ������
}
//дLCD����
//data:Ҫд���ֵ
void LCD_WR_DATA(__IO uint16_t data)
{
  data=data;			//ʹ��-O2�Ż���ʱ��,����������ʱ
  TFT_LCD->LCD_RAM=data;
}
//��LCD����
//����ֵ:������ֵ
uint16_t LCD_RD_DATA(void)
{
  __IO uint16_t ram;			//��ֹ���Ż�
  ram=TFT_LCD->LCD_RAM;
  return ram;
}
//д�Ĵ���
//LCD_Reg:�Ĵ�����ַ
//LCD_RegValue:Ҫд�������
void LCD_WriteReg(uint16_t LCD_Reg,uint16_t LCD_RegValue)
{
  TFT_LCD->LCD_REG = LCD_Reg;		//д��Ҫд�ļĴ������
  TFT_LCD->LCD_RAM = LCD_RegValue;//д������
}

//��mdk -O1ʱ���Ż�ʱ��Ҫ����
//��ʱi
void opt_delay(uint8_t i)
{
  while(i--);
}
//���Ĵ���
//LCD_Reg:�Ĵ�����ַ
//����ֵ:����������
uint16_t LCD_ReadReg(uint16_t LCD_Reg)
{
  LCD_WR_REG(LCD_Reg);		//д��Ҫ���ļĴ������
  opt_delay(5);
  return LCD_RD_DATA();		//���ض�����ֵ
}
//��ʼдGRAM
void LCD_WriteRAM_Prepare(void)
{
  TFT_LCD->LCD_REG=lcddev.wramcmd;
}
//LCDдGRAM
//RGB_Code:��ɫֵ
void LCD_WriteRAM(uint16_t RGB_Code)
{
  TFT_LCD->LCD_RAM = RGB_Code;//дʮ��λGRAM
}
//��ILI93xx����������ΪGBR��ʽ��������д���ʱ��ΪRGB��ʽ��
//ͨ���ú���ת��
//c:GBR��ʽ����ɫֵ
//����ֵ��RGB��ʽ����ɫֵ
uint16_t LCD_BGR2RGB(uint16_t c)
{
  uint16_t  r,g,b,rgb;
  b=(c>>0)&0x1f;
  g=(c>>5)&0x3f;
  r=(c>>11)&0x1f;
  rgb=(b<<11)+(g<<5)+(r<<0);
  return(rgb);
}
//��ȡ��ĳ�����ɫֵ
//x,y:����
//����ֵ:�˵����ɫ
uint16_t LCD_ReadPoint(uint16_t x,uint16_t y)
{
  __IO uint16_t r=0,g=0,b=0;
  if(x>=lcddev.width||y>=lcddev.height)return 0;	//�����˷�Χ,ֱ�ӷ���
  LCD_SetCursor(x,y);
  if(lcddev.id==0X9341||lcddev.id==0X6804||lcddev.id==0X5310||lcddev.id==0X1963)LCD_WR_REG(0X2E);//9341/6804/3510/1963 ���Ͷ�GRAMָ��
  else if(lcddev.id==0X5510)LCD_WR_REG(0X2E00);	//5510 ���Ͷ�GRAMָ��
  else LCD_WR_REG(0X22);      		 			//����IC���Ͷ�GRAMָ��
  if(lcddev.id==0X9320)opt_delay(2);				//FOR 9320,��ʱ2us
  r=LCD_RD_DATA();								//dummy Read
  if(lcddev.id==0X1963)return r;					//1963ֱ�Ӷ��Ϳ���
  opt_delay(2);
  r=LCD_RD_DATA();  		  						//ʵ��������ɫ
  if(lcddev.id==0X9341||lcddev.id==0X5310||lcddev.id==0X5510)		//9341/NT35310/NT35510Ҫ��2�ζ���
  {
    opt_delay(2);
    b=LCD_RD_DATA();
    g=r&0XFF;		//����9341/5310/5510,��һ�ζ�ȡ����RG��ֵ,R��ǰ,G�ں�,��ռ8λ
    g<<=8;
  }
  if(lcddev.id==0X9325||lcddev.id==0X4535||lcddev.id==0X4531||lcddev.id==0XB505||lcddev.id==0XC505)return r;	//�⼸��ICֱ�ӷ�����ɫֵ
  else if(lcddev.id==0X9341||lcddev.id==0X5310||lcddev.id==0X5510)return (((r>>11)<<11)|((g>>10)<<5)|(b>>11));//ILI9341/NT35310/NT35510��Ҫ��ʽת��һ��
  else return LCD_BGR2RGB(r);						//����IC
}
//LCD������ʾ
void LCD_DisplayOn(void)
{
  if(lcddev.id==0X9341||lcddev.id==0X5310||lcddev.id==0X1963)LCD_WR_REG(0X29);	//������ʾ
  else if(lcddev.id==0X5510)LCD_WR_REG(0X2900);	//������ʾ
}
//LCD�ر���ʾ
void LCD_DisplayOff(void)
{
  if(lcddev.id==0X9341||lcddev.id==0X5310||lcddev.id==0X1963)LCD_WR_REG(0X28);	//�ر���ʾ
  else if(lcddev.id==0X5510)LCD_WR_REG(0X2800);	//�ر���ʾ
}
//���ù��λ��(��RGB����Ч)
//Xpos:������
//Ypos:������
void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos)
{
  if(lcddev.id==0X9341||lcddev.id==0X5310)
  {
    LCD_WR_REG(lcddev.setxcmd);
    LCD_WR_DATA(Xpos>>8);LCD_WR_DATA(Xpos&0XFF);
    LCD_WR_REG(lcddev.setycmd);
    LCD_WR_DATA(Ypos>>8);LCD_WR_DATA(Ypos&0XFF);
  }else if(lcddev.id==0X1963)
  {
    if(lcddev.dir==0)//x������Ҫ�任
    {
      Xpos=lcddev.width-1-Xpos;
      LCD_WR_REG(lcddev.setxcmd);
      LCD_WR_DATA(0);LCD_WR_DATA(0);
      LCD_WR_DATA(Xpos>>8);LCD_WR_DATA(Xpos&0XFF);
    }else
    {
      LCD_WR_REG(lcddev.setxcmd);
      LCD_WR_DATA(Xpos>>8);LCD_WR_DATA(Xpos&0XFF);
      LCD_WR_DATA((lcddev.width-1)>>8);LCD_WR_DATA((lcddev.width-1)&0XFF);
    }
    LCD_WR_REG(lcddev.setycmd);
    LCD_WR_DATA(Ypos>>8);LCD_WR_DATA(Ypos&0XFF);
    LCD_WR_DATA((lcddev.height-1)>>8);LCD_WR_DATA((lcddev.height-1)&0XFF);

  }else if(lcddev.id==0X5510)
  {
    LCD_WR_REG(lcddev.setxcmd);LCD_WR_DATA(Xpos>>8);
    LCD_WR_REG(lcddev.setxcmd+1);LCD_WR_DATA(Xpos&0XFF);
    LCD_WR_REG(lcddev.setycmd);LCD_WR_DATA(Ypos>>8);
    LCD_WR_REG(lcddev.setycmd+1);LCD_WR_DATA(Ypos&0XFF);
  }
}
//����LCD���Զ�ɨ�跽��(��RGB����Ч)
//ע��:�����������ܻ��ܵ��˺������õ�Ӱ��(������9341),
//����,һ������ΪL2R_U2D����,�������Ϊ����ɨ�跽ʽ,���ܵ�����ʾ������.
//dir:0~7,����8������(���嶨���lcd.h)
//9341/5310/5510/1963��IC�Ѿ�ʵ�ʲ���
void LCD_Scan_Dir(uint8_t dir)
{
  uint16_t regval=0;
  uint16_t dirreg=0;
  uint16_t temp;
  if((lcddev.dir==1&&lcddev.id!=0X1963)||(lcddev.dir==0&&lcddev.id==0X1963))//����ʱ����1963���ı�ɨ�跽������ʱ1963�ı䷽��
  {
    switch(dir)//����ת��
    {
      case 0:dir=6;break;
      case 1:dir=7;break;
      case 2:dir=4;break;
      case 3:dir=5;break;
      case 4:dir=1;break;
      case 5:dir=0;break;
      case 6:dir=3;break;
      case 7:dir=2;break;
    }
  }
  if(lcddev.id==0x9341||lcddev.id==0X5310||lcddev.id==0X5510||lcddev.id==0X1963)//9341/5310/5510/1963,���⴦��
  {
    switch(dir)
    {
      case L2R_U2D://������,���ϵ���
        regval|=(0<<7)|(0<<6)|(0<<5);
        break;
      case L2R_D2U://������,���µ���
        regval|=(1<<7)|(0<<6)|(0<<5);
        break;
      case R2L_U2D://���ҵ���,���ϵ���
        regval|=(0<<7)|(1<<6)|(0<<5);
        break;
      case R2L_D2U://���ҵ���,���µ���
        regval|=(1<<7)|(1<<6)|(0<<5);
        break;
      case U2D_L2R://���ϵ���,������
        regval|=(0<<7)|(0<<6)|(1<<5);
        break;
      case U2D_R2L://���ϵ���,���ҵ���
        regval|=(0<<7)|(1<<6)|(1<<5);
        break;
      case D2U_L2R://���µ���,������
        regval|=(1<<7)|(0<<6)|(1<<5);
        break;
      case D2U_R2L://���µ���,���ҵ���
        regval|=(1<<7)|(1<<6)|(1<<5);
        break;
    }
    if(lcddev.id==0X5510)dirreg=0X3600;
    else dirreg=0X36;
    if((lcddev.id!=0X5310)&&(lcddev.id!=0X5510)&&(lcddev.id!=0X1963))regval|=0X08;//5310/5510/1963����ҪBGR
    LCD_WriteReg(dirreg,regval);
    if(lcddev.id!=0X1963)//1963�������괦��
    {
      if(regval&0X20)
      {
        if(lcddev.width<lcddev.height)//����X,Y
        {
          temp=lcddev.width;
          lcddev.width=lcddev.height;
          lcddev.height=temp;
        }
      }else
      {
        if(lcddev.width>lcddev.height)//����X,Y
        {
          temp=lcddev.width;
          lcddev.width=lcddev.height;
          lcddev.height=temp;
        }
      }
    }
    if(lcddev.id==0X5510)
    {
      LCD_WR_REG(lcddev.setxcmd);LCD_WR_DATA(0);
      LCD_WR_REG(lcddev.setxcmd+1);LCD_WR_DATA(0);
      LCD_WR_REG(lcddev.setxcmd+2);LCD_WR_DATA((lcddev.width-1)>>8);
      LCD_WR_REG(lcddev.setxcmd+3);LCD_WR_DATA((lcddev.width-1)&0XFF);
      LCD_WR_REG(lcddev.setycmd);LCD_WR_DATA(0);
      LCD_WR_REG(lcddev.setycmd+1);LCD_WR_DATA(0);
      LCD_WR_REG(lcddev.setycmd+2);LCD_WR_DATA((lcddev.height-1)>>8);
      LCD_WR_REG(lcddev.setycmd+3);LCD_WR_DATA((lcddev.height-1)&0XFF);
    }else
    {
      LCD_WR_REG(lcddev.setxcmd);
      LCD_WR_DATA(0);LCD_WR_DATA(0);
      LCD_WR_DATA((lcddev.width-1)>>8);LCD_WR_DATA((lcddev.width-1)&0XFF);
      LCD_WR_REG(lcddev.setycmd);
      LCD_WR_DATA(0);LCD_WR_DATA(0);
      LCD_WR_DATA((lcddev.height-1)>>8);LCD_WR_DATA((lcddev.height-1)&0XFF);
    }
  }
}
//����
//x,y:����
//POINT_COLOR:�˵����ɫ
void LCD_DrawPoint(uint16_t x,uint16_t y)
{
  LCD_SetCursor(x,y);		//���ù��λ��
  LCD_WriteRAM_Prepare();	//��ʼд��GRAM
  TFT_LCD->LCD_RAM=POINT_COLOR;
}
//���ٻ���
//x,y:����
//color:��ɫ
void LCD_Fast_DrawPoint(uint16_t x,uint16_t y,uint32_t color)
{
  if(lcddev.id==0X9341||lcddev.id==0X5310)
  {
    LCD_WR_REG(lcddev.setxcmd);
    LCD_WR_DATA(x>>8);LCD_WR_DATA(x&0XFF);
    LCD_WR_REG(lcddev.setycmd);
    LCD_WR_DATA(y>>8);LCD_WR_DATA(y&0XFF);
  }else if(lcddev.id==0X5510)
  {
    LCD_WR_REG(lcddev.setxcmd);LCD_WR_DATA(x>>8);
    LCD_WR_REG(lcddev.setxcmd+1);LCD_WR_DATA(x&0XFF);
    LCD_WR_REG(lcddev.setycmd);LCD_WR_DATA(y>>8);
    LCD_WR_REG(lcddev.setycmd+1);LCD_WR_DATA(y&0XFF);
  }else if(lcddev.id==0X1963)
  {
    if(lcddev.dir==0)x=lcddev.width-1-x;
    LCD_WR_REG(lcddev.setxcmd);
    LCD_WR_DATA(x>>8);LCD_WR_DATA(x&0XFF);
    LCD_WR_DATA(x>>8);LCD_WR_DATA(x&0XFF);
    LCD_WR_REG(lcddev.setycmd);
    LCD_WR_DATA(y>>8);LCD_WR_DATA(y&0XFF);
    LCD_WR_DATA(y>>8);LCD_WR_DATA(y&0XFF);
  }else if(lcddev.id==0X6804)
  {
    if(lcddev.dir==1)x=lcddev.width-1-x;//����ʱ����
    LCD_WR_REG(lcddev.setxcmd);
    LCD_WR_DATA(x>>8);LCD_WR_DATA(x&0XFF);
    LCD_WR_REG(lcddev.setycmd);
    LCD_WR_DATA(y>>8);LCD_WR_DATA(y&0XFF);
  }else
  {
    if(lcddev.dir==1)x=lcddev.width-1-x;//������ʵ���ǵ�תx,y����
    LCD_WriteReg(lcddev.setxcmd,x);
    LCD_WriteReg(lcddev.setycmd,y);
  }
  TFT_LCD->LCD_REG=lcddev.wramcmd;
  TFT_LCD->LCD_RAM=color;
}
//SSD1963 ��������
//pwm:����ȼ�,0~100.Խ��Խ��.
void LCD_SSD_BackLightSet(uint8_t pwm)
{
  LCD_WR_REG(0xBE);	//����PWM���
  LCD_WR_DATA(0x05);	//1����PWMƵ��
  LCD_WR_DATA(pwm*2.55);//2����PWMռ�ձ�
  LCD_WR_DATA(0x01);	//3����C
  LCD_WR_DATA(0xFF);	//4����D
  LCD_WR_DATA(0x00);	//5����E
  LCD_WR_DATA(0x00);	//6����F
}

//����LCD��ʾ����
//dir:0,������1,����
void LCD_Display_Dir(uint8_t dir)
{
  lcddev.dir=dir;		//����/����
  if(dir==0)			//����
  {
    lcddev.width=240;
    lcddev.height=320;
    if(lcddev.id==0X9341||lcddev.id==0X5310)
    {
      lcddev.wramcmd=0X2C;
      lcddev.setxcmd=0X2A;
      lcddev.setycmd=0X2B;
      if(lcddev.id==0X5310)
      {
        lcddev.width=320;
        lcddev.height=480;
      }
    }else if(lcddev.id==0x5510)
    {
      lcddev.wramcmd=0X2C00;
      lcddev.setxcmd=0X2A00;
      lcddev.setycmd=0X2B00;
      lcddev.width=480;
      lcddev.height=800;
    }else if(lcddev.id==0X1963)
    {
      lcddev.wramcmd=0X2C;	//����д��GRAM��ָ��
      lcddev.setxcmd=0X2B;	//����дX����ָ��
      lcddev.setycmd=0X2A;	//����дY����ָ��
      lcddev.width=480;		//���ÿ��480
      lcddev.height=800;		//���ø߶�800
    }
  }else 				//����
  {
    lcddev.width=320;
    lcddev.height=240;
    if(lcddev.id==0X9341||lcddev.id==0X5310)
    {
      lcddev.wramcmd=0X2C;
      lcddev.setxcmd=0X2A;
      lcddev.setycmd=0X2B;
    }else if(lcddev.id==0x5510)
    {
      lcddev.wramcmd=0X2C00;
      lcddev.setxcmd=0X2A00;
      lcddev.setycmd=0X2B00;
      lcddev.width=800;
      lcddev.height=480;
    }else if(lcddev.id==0X1963)
    {
      lcddev.wramcmd=0X2C;	//����д��GRAM��ָ��
      lcddev.setxcmd=0X2A;	//����дX����ָ��
      lcddev.setycmd=0X2B;	//����дY����ָ��
      lcddev.width=800;		//���ÿ��800
      lcddev.height=480;		//���ø߶�480
    }
    if(lcddev.id==0X5310)
    {
      lcddev.width=480;
      lcddev.height=320;
    }
  }
  LCD_Scan_Dir(DFT_SCAN_DIR);	//Ĭ��ɨ�跽��
}
//���ô���(��RGB����Ч),���Զ����û������굽�������Ͻ�(sx,sy).
//sx,sy:������ʼ����(���Ͻ�)
//width,height:���ڿ�Ⱥ͸߶�,�������0!!
//�����С:width*height.
void LCD_Set_Window(uint16_t sx,uint16_t sy,uint16_t width,uint16_t height)
{
  uint16_t twidth,theight;
  twidth=sx+width-1;
  theight=sy+height-1;
  if(lcddev.id==0X9341||lcddev.id==0X5310||(lcddev.dir==1&&lcddev.id==0X1963))
  {
    LCD_WR_REG(lcddev.setxcmd);
    LCD_WR_DATA(sx>>8);
    LCD_WR_DATA(sx&0XFF);
    LCD_WR_DATA(twidth>>8);
    LCD_WR_DATA(twidth&0XFF);
    LCD_WR_REG(lcddev.setycmd);
    LCD_WR_DATA(sy>>8);
    LCD_WR_DATA(sy&0XFF);
    LCD_WR_DATA(theight>>8);
    LCD_WR_DATA(theight&0XFF);
  }else if(lcddev.id==0X1963)//1963�������⴦��
  {
    sx=lcddev.width-width-sx;
    height=sy+height-1;
    LCD_WR_REG(lcddev.setxcmd);
    LCD_WR_DATA(sx>>8);
    LCD_WR_DATA(sx&0XFF);
    LCD_WR_DATA((sx+width-1)>>8);
    LCD_WR_DATA((sx+width-1)&0XFF);
    LCD_WR_REG(lcddev.setycmd);
    LCD_WR_DATA(sy>>8);
    LCD_WR_DATA(sy&0XFF);
    LCD_WR_DATA(height>>8);
    LCD_WR_DATA(height&0XFF);
  }else if(lcddev.id==0X5510)
  {
    LCD_WR_REG(lcddev.setxcmd);LCD_WR_DATA(sx>>8);
    LCD_WR_REG(lcddev.setxcmd+1);LCD_WR_DATA(sx&0XFF);
    LCD_WR_REG(lcddev.setxcmd+2);LCD_WR_DATA(twidth>>8);
    LCD_WR_REG(lcddev.setxcmd+3);LCD_WR_DATA(twidth&0XFF);
    LCD_WR_REG(lcddev.setycmd);LCD_WR_DATA(sy>>8);
    LCD_WR_REG(lcddev.setycmd+1);LCD_WR_DATA(sy&0XFF);
    LCD_WR_REG(lcddev.setycmd+2);LCD_WR_DATA(theight>>8);
    LCD_WR_REG(lcddev.setycmd+3);LCD_WR_DATA(theight&0XFF);
  }
}


//��ʼ��lcd
//�ó�ʼ���������Գ�ʼ�������ͺŵ�LCD(�����.c�ļ���ǰ�������)
void TFTLCD_Init(void)
{
  HAL_Delay(50); // delay 50 ms

  //����9341 ID�Ķ�ȡ
  LCD_WR_REG(0XD3);
  lcddev.id=LCD_RD_DATA();	//dummy read
  lcddev.id=LCD_RD_DATA();	//����0X00
  lcddev.id=LCD_RD_DATA();   	//��ȡ93
  lcddev.id<<=8;
  lcddev.id|=LCD_RD_DATA();  	//��ȡ41
  if(lcddev.id!=0X9341)		//��9341,���Կ����ǲ���NT35310
  {
    LCD_WR_REG(0XD4);
    lcddev.id=LCD_RD_DATA();//dummy read
    lcddev.id=LCD_RD_DATA();//����0X01
    lcddev.id=LCD_RD_DATA();//����0X53
    lcddev.id<<=8;
    lcddev.id|=LCD_RD_DATA();	//�������0X10
    if(lcddev.id!=0X5310)		//Ҳ����NT35310,���Կ����ǲ���NT35510
    {
      LCD_WR_REG(0XDA00);
      lcddev.id=LCD_RD_DATA();		//����0X00
      LCD_WR_REG(0XDB00);
      lcddev.id=LCD_RD_DATA();		//����0X80
      lcddev.id<<=8;
      LCD_WR_REG(0XDC00);
      lcddev.id|=LCD_RD_DATA();		//����0X00
      if(lcddev.id==0x8000)lcddev.id=0x5510;//NT35510���ص�ID��8000H,Ϊ��������,����ǿ������Ϊ5510
      if(lcddev.id!=0X5510)			//Ҳ����NT5510,���Կ����ǲ���SSD1963
      {
        LCD_WR_REG(0XA1);
        lcddev.id=LCD_RD_DATA();
        lcddev.id=LCD_RD_DATA();	//����0X57
        lcddev.id<<=8;
        lcddev.id|=LCD_RD_DATA();	//����0X61
        if(lcddev.id==0X5761)lcddev.id=0X1963;//SSD1963���ص�ID��5761H,Ϊ��������,����ǿ������Ϊ1963
      }
    }
  }
//  printf(" LCD ID:%x\r\n",lcddev.id); //��ӡLCD ID
  print_UARTx(huart1, " LCD ID:%x\r\n",lcddev.id);
  if(lcddev.id==0X9341)	//9341��ʼ��
  {
    LCD_WR_REG(0xCF);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xC1);
    LCD_WR_DATA(0X30);
    LCD_WR_REG(0xED);
    LCD_WR_DATA(0x64);
    LCD_WR_DATA(0x03);
    LCD_WR_DATA(0X12);
    LCD_WR_DATA(0X81);
    LCD_WR_REG(0xE8);
    LCD_WR_DATA(0x85);
    LCD_WR_DATA(0x10);
    LCD_WR_DATA(0x7A);
    LCD_WR_REG(0xCB);
    LCD_WR_DATA(0x39);
    LCD_WR_DATA(0x2C);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x34);
    LCD_WR_DATA(0x02);
    LCD_WR_REG(0xF7);
    LCD_WR_DATA(0x20);
    LCD_WR_REG(0xEA);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_REG(0xC0);    //Power control
    LCD_WR_DATA(0x1B);   //VRH[5:0]
    LCD_WR_REG(0xC1);    //Power control
    LCD_WR_DATA(0x01);   //SAP[2:0];BT[3:0]
    LCD_WR_REG(0xC5);    //VCM control
    LCD_WR_DATA(0x30); 	 //3F
    LCD_WR_DATA(0x30); 	 //3C
    LCD_WR_REG(0xC7);    //VCM control2
    LCD_WR_DATA(0XB7);
    LCD_WR_REG(0x36);    // Memory Access Control
    LCD_WR_DATA(0x48);
    LCD_WR_REG(0x3A);
    LCD_WR_DATA(0x55);
    LCD_WR_REG(0xB1);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x1A);
    LCD_WR_REG(0xB6);    // Display Function Control
    LCD_WR_DATA(0x0A);
    LCD_WR_DATA(0xA2);
    LCD_WR_REG(0xF2);    // 3Gamma Function Disable
    LCD_WR_DATA(0x00);
    LCD_WR_REG(0x26);    //Gamma curve selected
    LCD_WR_DATA(0x01);
    LCD_WR_REG(0xE0);    //Set Gamma
    LCD_WR_DATA(0x0F);
    LCD_WR_DATA(0x2A);
    LCD_WR_DATA(0x28);
    LCD_WR_DATA(0x08);
    LCD_WR_DATA(0x0E);
    LCD_WR_DATA(0x08);
    LCD_WR_DATA(0x54);
    LCD_WR_DATA(0XA9);
    LCD_WR_DATA(0x43);
    LCD_WR_DATA(0x0A);
    LCD_WR_DATA(0x0F);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_REG(0XE1);    //Set Gamma
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x15);
    LCD_WR_DATA(0x17);
    LCD_WR_DATA(0x07);
    LCD_WR_DATA(0x11);
    LCD_WR_DATA(0x06);
    LCD_WR_DATA(0x2B);
    LCD_WR_DATA(0x56);
    LCD_WR_DATA(0x3C);
    LCD_WR_DATA(0x05);
    LCD_WR_DATA(0x10);
    LCD_WR_DATA(0x0F);
    LCD_WR_DATA(0x3F);
    LCD_WR_DATA(0x3F);
    LCD_WR_DATA(0x0F);
    LCD_WR_REG(0x2B);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x01);
    LCD_WR_DATA(0x3f);
    LCD_WR_REG(0x2A);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xef);
    LCD_WR_REG(0x11); //Exit Sleep
    HAL_Delay(120);
    LCD_WR_REG(0x29); //display on
  }else if(lcddev.id==0x5310)
  {
    LCD_WR_REG(0xED);
    LCD_WR_DATA(0x01);
    LCD_WR_DATA(0xFE);

    LCD_WR_REG(0xEE);
    LCD_WR_DATA(0xDE);
    LCD_WR_DATA(0x21);

    LCD_WR_REG(0xF1);
    LCD_WR_DATA(0x01);
    LCD_WR_REG(0xDF);
    LCD_WR_DATA(0x10);

    //VCOMvoltage//
    LCD_WR_REG(0xC4);
    LCD_WR_DATA(0x8F);	  //5f

    LCD_WR_REG(0xC6);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xE2);
    LCD_WR_DATA(0xE2);
    LCD_WR_DATA(0xE2);
    LCD_WR_REG(0xBF);
    LCD_WR_DATA(0xAA);

    LCD_WR_REG(0xB0);
    LCD_WR_DATA(0x0D);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x0D);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x11);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x19);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x21);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x2D);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x3D);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x5D);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x5D);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xB1);
    LCD_WR_DATA(0x80);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x8B);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x96);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xB2);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x02);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x03);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xB3);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xB4);
    LCD_WR_DATA(0x8B);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x96);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xA1);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xB5);
    LCD_WR_DATA(0x02);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x03);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x04);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xB6);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xB7);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x3F);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x5E);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x64);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x8C);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xAC);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xDC);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x70);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x90);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xEB);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xDC);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xB8);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xBA);
    LCD_WR_DATA(0x24);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xC1);
    LCD_WR_DATA(0x20);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x54);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xFF);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xC2);
    LCD_WR_DATA(0x0A);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x04);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xC3);
    LCD_WR_DATA(0x3C);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x3A);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x39);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x37);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x3C);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x36);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x32);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x2F);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x2C);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x29);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x26);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x24);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x24);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x23);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x3C);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x36);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x32);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x2F);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x2C);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x29);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x26);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x24);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x24);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x23);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xC4);
    LCD_WR_DATA(0x62);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x05);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x84);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xF0);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x18);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xA4);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x18);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x50);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x0C);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x17);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x95);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xF3);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xE6);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xC5);
    LCD_WR_DATA(0x32);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x44);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x65);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x76);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x88);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xC6);
    LCD_WR_DATA(0x20);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x17);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x01);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xC7);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xC8);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xC9);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xE0);
    LCD_WR_DATA(0x16);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x1C);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x21);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x36);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x46);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x52);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x64);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x7A);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x8B);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x99);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xA8);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xB9);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xC4);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xCA);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xD2);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xD9);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xE0);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xF3);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xE1);
    LCD_WR_DATA(0x16);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x1C);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x22);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x36);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x45);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x52);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x64);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x7A);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x8B);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x99);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xA8);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xB9);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xC4);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xCA);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xD2);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xD8);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xE0);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xF3);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xE2);
    LCD_WR_DATA(0x05);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x0B);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x1B);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x34);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x44);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x4F);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x61);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x79);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x88);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x97);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xA6);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xB7);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xC2);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xC7);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xD1);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xD6);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xDD);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xF3);
    LCD_WR_DATA(0x00);
    LCD_WR_REG(0xE3);
    LCD_WR_DATA(0x05);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xA);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x1C);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x33);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x44);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x50);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x62);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x78);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x88);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x97);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xA6);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xB7);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xC2);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xC7);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xD1);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xD5);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xDD);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xF3);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xE4);
    LCD_WR_DATA(0x01);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x01);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x02);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x2A);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x3C);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x4B);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x5D);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x74);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x84);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x93);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xA2);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xB3);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xBE);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xC4);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xCD);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xD3);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xDD);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xF3);
    LCD_WR_DATA(0x00);
    LCD_WR_REG(0xE5);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x02);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x29);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x3C);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x4B);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x5D);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x74);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x84);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x93);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xA2);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xB3);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xBE);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xC4);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xCD);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xD3);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xDC);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xF3);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xE6);
    LCD_WR_DATA(0x11);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x34);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x56);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x76);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x77);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x66);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x88);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x99);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xBB);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x99);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x66);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x55);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x55);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x45);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x43);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x44);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xE7);
    LCD_WR_DATA(0x32);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x55);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x76);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x66);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x67);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x67);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x87);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x99);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xBB);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x99);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x77);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x44);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x56);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x23);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x33);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x45);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xE8);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x99);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x87);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x88);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x77);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x66);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x88);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xAA);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xBB);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x99);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x66);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x55);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x55);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x44);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x44);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x55);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xE9);
    LCD_WR_DATA(0xAA);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0x00);
    LCD_WR_DATA(0xAA);

    LCD_WR_REG(0xCF);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xF0);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x50);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xF3);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xF9);
    LCD_WR_DATA(0x06);
    LCD_WR_DATA(0x10);
    LCD_WR_DATA(0x29);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0x3A);
    LCD_WR_DATA(0x55);	//66

    LCD_WR_REG(0x11);
    HAL_Delay(100);
    LCD_WR_REG(0x29);
    LCD_WR_REG(0x35);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0x51);
    LCD_WR_DATA(0xFF);
    LCD_WR_REG(0x53);
    LCD_WR_DATA(0x2C);
    LCD_WR_REG(0x55);
    LCD_WR_DATA(0x82);
    LCD_WR_REG(0x2c);
  }else if(lcddev.id==0x5510)
  {
    LCD_WriteReg(0xF000,0x55);
    LCD_WriteReg(0xF001,0xAA);
    LCD_WriteReg(0xF002,0x52);
    LCD_WriteReg(0xF003,0x08);
    LCD_WriteReg(0xF004,0x01);
    //AVDD Set AVDD 5.2V
    LCD_WriteReg(0xB000,0x0D);
    LCD_WriteReg(0xB001,0x0D);
    LCD_WriteReg(0xB002,0x0D);
    //AVDD ratio
    LCD_WriteReg(0xB600,0x34);
    LCD_WriteReg(0xB601,0x34);
    LCD_WriteReg(0xB602,0x34);
    //AVEE -5.2V
    LCD_WriteReg(0xB100,0x0D);
    LCD_WriteReg(0xB101,0x0D);
    LCD_WriteReg(0xB102,0x0D);
    //AVEE ratio
    LCD_WriteReg(0xB700,0x34);
    LCD_WriteReg(0xB701,0x34);
    LCD_WriteReg(0xB702,0x34);
    //VCL -2.5V
    LCD_WriteReg(0xB200,0x00);
    LCD_WriteReg(0xB201,0x00);
    LCD_WriteReg(0xB202,0x00);
    //VCL ratio
    LCD_WriteReg(0xB800,0x24);
    LCD_WriteReg(0xB801,0x24);
    LCD_WriteReg(0xB802,0x24);
    //VGH 15V (Free pump)
    LCD_WriteReg(0xBF00,0x01);
    LCD_WriteReg(0xB300,0x0F);
    LCD_WriteReg(0xB301,0x0F);
    LCD_WriteReg(0xB302,0x0F);
    //VGH ratio
    LCD_WriteReg(0xB900,0x34);
    LCD_WriteReg(0xB901,0x34);
    LCD_WriteReg(0xB902,0x34);
    //VGL_REG -10V
    LCD_WriteReg(0xB500,0x08);
    LCD_WriteReg(0xB501,0x08);
    LCD_WriteReg(0xB502,0x08);
    LCD_WriteReg(0xC200,0x03);
    //VGLX ratio
    LCD_WriteReg(0xBA00,0x24);
    LCD_WriteReg(0xBA01,0x24);
    LCD_WriteReg(0xBA02,0x24);
    //VGMP/VGSP 4.5V/0V
    LCD_WriteReg(0xBC00,0x00);
    LCD_WriteReg(0xBC01,0x78);
    LCD_WriteReg(0xBC02,0x00);
    //VGMN/VGSN -4.5V/0V
    LCD_WriteReg(0xBD00,0x00);
    LCD_WriteReg(0xBD01,0x78);
    LCD_WriteReg(0xBD02,0x00);
    //VCOM
    LCD_WriteReg(0xBE00,0x00);
    LCD_WriteReg(0xBE01,0x64);
    //Gamma Setting
    LCD_WriteReg(0xD100,0x00);
    LCD_WriteReg(0xD101,0x33);
    LCD_WriteReg(0xD102,0x00);
    LCD_WriteReg(0xD103,0x34);
    LCD_WriteReg(0xD104,0x00);
    LCD_WriteReg(0xD105,0x3A);
    LCD_WriteReg(0xD106,0x00);
    LCD_WriteReg(0xD107,0x4A);
    LCD_WriteReg(0xD108,0x00);
    LCD_WriteReg(0xD109,0x5C);
    LCD_WriteReg(0xD10A,0x00);
    LCD_WriteReg(0xD10B,0x81);
    LCD_WriteReg(0xD10C,0x00);
    LCD_WriteReg(0xD10D,0xA6);
    LCD_WriteReg(0xD10E,0x00);
    LCD_WriteReg(0xD10F,0xE5);
    LCD_WriteReg(0xD110,0x01);
    LCD_WriteReg(0xD111,0x13);
    LCD_WriteReg(0xD112,0x01);
    LCD_WriteReg(0xD113,0x54);
    LCD_WriteReg(0xD114,0x01);
    LCD_WriteReg(0xD115,0x82);
    LCD_WriteReg(0xD116,0x01);
    LCD_WriteReg(0xD117,0xCA);
    LCD_WriteReg(0xD118,0x02);
    LCD_WriteReg(0xD119,0x00);
    LCD_WriteReg(0xD11A,0x02);
    LCD_WriteReg(0xD11B,0x01);
    LCD_WriteReg(0xD11C,0x02);
    LCD_WriteReg(0xD11D,0x34);
    LCD_WriteReg(0xD11E,0x02);
    LCD_WriteReg(0xD11F,0x67);
    LCD_WriteReg(0xD120,0x02);
    LCD_WriteReg(0xD121,0x84);
    LCD_WriteReg(0xD122,0x02);
    LCD_WriteReg(0xD123,0xA4);
    LCD_WriteReg(0xD124,0x02);
    LCD_WriteReg(0xD125,0xB7);
    LCD_WriteReg(0xD126,0x02);
    LCD_WriteReg(0xD127,0xCF);
    LCD_WriteReg(0xD128,0x02);
    LCD_WriteReg(0xD129,0xDE);
    LCD_WriteReg(0xD12A,0x02);
    LCD_WriteReg(0xD12B,0xF2);
    LCD_WriteReg(0xD12C,0x02);
    LCD_WriteReg(0xD12D,0xFE);
    LCD_WriteReg(0xD12E,0x03);
    LCD_WriteReg(0xD12F,0x10);
    LCD_WriteReg(0xD130,0x03);
    LCD_WriteReg(0xD131,0x33);
    LCD_WriteReg(0xD132,0x03);
    LCD_WriteReg(0xD133,0x6D);
    LCD_WriteReg(0xD200,0x00);
    LCD_WriteReg(0xD201,0x33);
    LCD_WriteReg(0xD202,0x00);
    LCD_WriteReg(0xD203,0x34);
    LCD_WriteReg(0xD204,0x00);
    LCD_WriteReg(0xD205,0x3A);
    LCD_WriteReg(0xD206,0x00);
    LCD_WriteReg(0xD207,0x4A);
    LCD_WriteReg(0xD208,0x00);
    LCD_WriteReg(0xD209,0x5C);
    LCD_WriteReg(0xD20A,0x00);

    LCD_WriteReg(0xD20B,0x81);
    LCD_WriteReg(0xD20C,0x00);
    LCD_WriteReg(0xD20D,0xA6);
    LCD_WriteReg(0xD20E,0x00);
    LCD_WriteReg(0xD20F,0xE5);
    LCD_WriteReg(0xD210,0x01);
    LCD_WriteReg(0xD211,0x13);
    LCD_WriteReg(0xD212,0x01);
    LCD_WriteReg(0xD213,0x54);
    LCD_WriteReg(0xD214,0x01);
    LCD_WriteReg(0xD215,0x82);
    LCD_WriteReg(0xD216,0x01);
    LCD_WriteReg(0xD217,0xCA);
    LCD_WriteReg(0xD218,0x02);
    LCD_WriteReg(0xD219,0x00);
    LCD_WriteReg(0xD21A,0x02);
    LCD_WriteReg(0xD21B,0x01);
    LCD_WriteReg(0xD21C,0x02);
    LCD_WriteReg(0xD21D,0x34);
    LCD_WriteReg(0xD21E,0x02);
    LCD_WriteReg(0xD21F,0x67);
    LCD_WriteReg(0xD220,0x02);
    LCD_WriteReg(0xD221,0x84);
    LCD_WriteReg(0xD222,0x02);
    LCD_WriteReg(0xD223,0xA4);
    LCD_WriteReg(0xD224,0x02);
    LCD_WriteReg(0xD225,0xB7);
    LCD_WriteReg(0xD226,0x02);
    LCD_WriteReg(0xD227,0xCF);
    LCD_WriteReg(0xD228,0x02);
    LCD_WriteReg(0xD229,0xDE);
    LCD_WriteReg(0xD22A,0x02);
    LCD_WriteReg(0xD22B,0xF2);
    LCD_WriteReg(0xD22C,0x02);
    LCD_WriteReg(0xD22D,0xFE);
    LCD_WriteReg(0xD22E,0x03);
    LCD_WriteReg(0xD22F,0x10);
    LCD_WriteReg(0xD230,0x03);
    LCD_WriteReg(0xD231,0x33);
    LCD_WriteReg(0xD232,0x03);
    LCD_WriteReg(0xD233,0x6D);
    LCD_WriteReg(0xD300,0x00);
    LCD_WriteReg(0xD301,0x33);
    LCD_WriteReg(0xD302,0x00);
    LCD_WriteReg(0xD303,0x34);
    LCD_WriteReg(0xD304,0x00);
    LCD_WriteReg(0xD305,0x3A);
    LCD_WriteReg(0xD306,0x00);
    LCD_WriteReg(0xD307,0x4A);
    LCD_WriteReg(0xD308,0x00);
    LCD_WriteReg(0xD309,0x5C);
    LCD_WriteReg(0xD30A,0x00);

    LCD_WriteReg(0xD30B,0x81);
    LCD_WriteReg(0xD30C,0x00);
    LCD_WriteReg(0xD30D,0xA6);
    LCD_WriteReg(0xD30E,0x00);
    LCD_WriteReg(0xD30F,0xE5);
    LCD_WriteReg(0xD310,0x01);
    LCD_WriteReg(0xD311,0x13);
    LCD_WriteReg(0xD312,0x01);
    LCD_WriteReg(0xD313,0x54);
    LCD_WriteReg(0xD314,0x01);
    LCD_WriteReg(0xD315,0x82);
    LCD_WriteReg(0xD316,0x01);
    LCD_WriteReg(0xD317,0xCA);
    LCD_WriteReg(0xD318,0x02);
    LCD_WriteReg(0xD319,0x00);
    LCD_WriteReg(0xD31A,0x02);
    LCD_WriteReg(0xD31B,0x01);
    LCD_WriteReg(0xD31C,0x02);
    LCD_WriteReg(0xD31D,0x34);
    LCD_WriteReg(0xD31E,0x02);
    LCD_WriteReg(0xD31F,0x67);
    LCD_WriteReg(0xD320,0x02);
    LCD_WriteReg(0xD321,0x84);
    LCD_WriteReg(0xD322,0x02);
    LCD_WriteReg(0xD323,0xA4);
    LCD_WriteReg(0xD324,0x02);
    LCD_WriteReg(0xD325,0xB7);
    LCD_WriteReg(0xD326,0x02);
    LCD_WriteReg(0xD327,0xCF);
    LCD_WriteReg(0xD328,0x02);
    LCD_WriteReg(0xD329,0xDE);
    LCD_WriteReg(0xD32A,0x02);
    LCD_WriteReg(0xD32B,0xF2);
    LCD_WriteReg(0xD32C,0x02);
    LCD_WriteReg(0xD32D,0xFE);
    LCD_WriteReg(0xD32E,0x03);
    LCD_WriteReg(0xD32F,0x10);
    LCD_WriteReg(0xD330,0x03);
    LCD_WriteReg(0xD331,0x33);
    LCD_WriteReg(0xD332,0x03);
    LCD_WriteReg(0xD333,0x6D);
    LCD_WriteReg(0xD400,0x00);
    LCD_WriteReg(0xD401,0x33);
    LCD_WriteReg(0xD402,0x00);
    LCD_WriteReg(0xD403,0x34);
    LCD_WriteReg(0xD404,0x00);
    LCD_WriteReg(0xD405,0x3A);
    LCD_WriteReg(0xD406,0x00);
    LCD_WriteReg(0xD407,0x4A);
    LCD_WriteReg(0xD408,0x00);
    LCD_WriteReg(0xD409,0x5C);
    LCD_WriteReg(0xD40A,0x00);
    LCD_WriteReg(0xD40B,0x81);

    LCD_WriteReg(0xD40C,0x00);
    LCD_WriteReg(0xD40D,0xA6);
    LCD_WriteReg(0xD40E,0x00);
    LCD_WriteReg(0xD40F,0xE5);
    LCD_WriteReg(0xD410,0x01);
    LCD_WriteReg(0xD411,0x13);
    LCD_WriteReg(0xD412,0x01);
    LCD_WriteReg(0xD413,0x54);
    LCD_WriteReg(0xD414,0x01);
    LCD_WriteReg(0xD415,0x82);
    LCD_WriteReg(0xD416,0x01);
    LCD_WriteReg(0xD417,0xCA);
    LCD_WriteReg(0xD418,0x02);
    LCD_WriteReg(0xD419,0x00);
    LCD_WriteReg(0xD41A,0x02);
    LCD_WriteReg(0xD41B,0x01);
    LCD_WriteReg(0xD41C,0x02);
    LCD_WriteReg(0xD41D,0x34);
    LCD_WriteReg(0xD41E,0x02);
    LCD_WriteReg(0xD41F,0x67);
    LCD_WriteReg(0xD420,0x02);
    LCD_WriteReg(0xD421,0x84);
    LCD_WriteReg(0xD422,0x02);
    LCD_WriteReg(0xD423,0xA4);
    LCD_WriteReg(0xD424,0x02);
    LCD_WriteReg(0xD425,0xB7);
    LCD_WriteReg(0xD426,0x02);
    LCD_WriteReg(0xD427,0xCF);
    LCD_WriteReg(0xD428,0x02);
    LCD_WriteReg(0xD429,0xDE);
    LCD_WriteReg(0xD42A,0x02);
    LCD_WriteReg(0xD42B,0xF2);
    LCD_WriteReg(0xD42C,0x02);
    LCD_WriteReg(0xD42D,0xFE);
    LCD_WriteReg(0xD42E,0x03);
    LCD_WriteReg(0xD42F,0x10);
    LCD_WriteReg(0xD430,0x03);
    LCD_WriteReg(0xD431,0x33);
    LCD_WriteReg(0xD432,0x03);
    LCD_WriteReg(0xD433,0x6D);
    LCD_WriteReg(0xD500,0x00);
    LCD_WriteReg(0xD501,0x33);
    LCD_WriteReg(0xD502,0x00);
    LCD_WriteReg(0xD503,0x34);
    LCD_WriteReg(0xD504,0x00);
    LCD_WriteReg(0xD505,0x3A);
    LCD_WriteReg(0xD506,0x00);
    LCD_WriteReg(0xD507,0x4A);
    LCD_WriteReg(0xD508,0x00);
    LCD_WriteReg(0xD509,0x5C);
    LCD_WriteReg(0xD50A,0x00);
    LCD_WriteReg(0xD50B,0x81);

    LCD_WriteReg(0xD50C,0x00);
    LCD_WriteReg(0xD50D,0xA6);
    LCD_WriteReg(0xD50E,0x00);
    LCD_WriteReg(0xD50F,0xE5);
    LCD_WriteReg(0xD510,0x01);
    LCD_WriteReg(0xD511,0x13);
    LCD_WriteReg(0xD512,0x01);
    LCD_WriteReg(0xD513,0x54);
    LCD_WriteReg(0xD514,0x01);
    LCD_WriteReg(0xD515,0x82);
    LCD_WriteReg(0xD516,0x01);
    LCD_WriteReg(0xD517,0xCA);
    LCD_WriteReg(0xD518,0x02);
    LCD_WriteReg(0xD519,0x00);
    LCD_WriteReg(0xD51A,0x02);
    LCD_WriteReg(0xD51B,0x01);
    LCD_WriteReg(0xD51C,0x02);
    LCD_WriteReg(0xD51D,0x34);
    LCD_WriteReg(0xD51E,0x02);
    LCD_WriteReg(0xD51F,0x67);
    LCD_WriteReg(0xD520,0x02);
    LCD_WriteReg(0xD521,0x84);
    LCD_WriteReg(0xD522,0x02);
    LCD_WriteReg(0xD523,0xA4);
    LCD_WriteReg(0xD524,0x02);
    LCD_WriteReg(0xD525,0xB7);
    LCD_WriteReg(0xD526,0x02);
    LCD_WriteReg(0xD527,0xCF);
    LCD_WriteReg(0xD528,0x02);
    LCD_WriteReg(0xD529,0xDE);
    LCD_WriteReg(0xD52A,0x02);
    LCD_WriteReg(0xD52B,0xF2);
    LCD_WriteReg(0xD52C,0x02);
    LCD_WriteReg(0xD52D,0xFE);
    LCD_WriteReg(0xD52E,0x03);
    LCD_WriteReg(0xD52F,0x10);
    LCD_WriteReg(0xD530,0x03);
    LCD_WriteReg(0xD531,0x33);
    LCD_WriteReg(0xD532,0x03);
    LCD_WriteReg(0xD533,0x6D);
    LCD_WriteReg(0xD600,0x00);
    LCD_WriteReg(0xD601,0x33);
    LCD_WriteReg(0xD602,0x00);
    LCD_WriteReg(0xD603,0x34);
    LCD_WriteReg(0xD604,0x00);
    LCD_WriteReg(0xD605,0x3A);
    LCD_WriteReg(0xD606,0x00);
    LCD_WriteReg(0xD607,0x4A);
    LCD_WriteReg(0xD608,0x00);
    LCD_WriteReg(0xD609,0x5C);
    LCD_WriteReg(0xD60A,0x00);
    LCD_WriteReg(0xD60B,0x81);

    LCD_WriteReg(0xD60C,0x00);
    LCD_WriteReg(0xD60D,0xA6);
    LCD_WriteReg(0xD60E,0x00);
    LCD_WriteReg(0xD60F,0xE5);
    LCD_WriteReg(0xD610,0x01);
    LCD_WriteReg(0xD611,0x13);
    LCD_WriteReg(0xD612,0x01);
    LCD_WriteReg(0xD613,0x54);
    LCD_WriteReg(0xD614,0x01);
    LCD_WriteReg(0xD615,0x82);
    LCD_WriteReg(0xD616,0x01);
    LCD_WriteReg(0xD617,0xCA);
    LCD_WriteReg(0xD618,0x02);
    LCD_WriteReg(0xD619,0x00);
    LCD_WriteReg(0xD61A,0x02);
    LCD_WriteReg(0xD61B,0x01);
    LCD_WriteReg(0xD61C,0x02);
    LCD_WriteReg(0xD61D,0x34);
    LCD_WriteReg(0xD61E,0x02);
    LCD_WriteReg(0xD61F,0x67);
    LCD_WriteReg(0xD620,0x02);
    LCD_WriteReg(0xD621,0x84);
    LCD_WriteReg(0xD622,0x02);
    LCD_WriteReg(0xD623,0xA4);
    LCD_WriteReg(0xD624,0x02);
    LCD_WriteReg(0xD625,0xB7);
    LCD_WriteReg(0xD626,0x02);
    LCD_WriteReg(0xD627,0xCF);
    LCD_WriteReg(0xD628,0x02);
    LCD_WriteReg(0xD629,0xDE);
    LCD_WriteReg(0xD62A,0x02);
    LCD_WriteReg(0xD62B,0xF2);
    LCD_WriteReg(0xD62C,0x02);
    LCD_WriteReg(0xD62D,0xFE);
    LCD_WriteReg(0xD62E,0x03);
    LCD_WriteReg(0xD62F,0x10);
    LCD_WriteReg(0xD630,0x03);
    LCD_WriteReg(0xD631,0x33);
    LCD_WriteReg(0xD632,0x03);
    LCD_WriteReg(0xD633,0x6D);
    //LV2 Page 0 enable
    LCD_WriteReg(0xF000,0x55);
    LCD_WriteReg(0xF001,0xAA);
    LCD_WriteReg(0xF002,0x52);
    LCD_WriteReg(0xF003,0x08);
    LCD_WriteReg(0xF004,0x00);
    //Display control
    LCD_WriteReg(0xB100, 0xCC);
    LCD_WriteReg(0xB101, 0x00);
    //Source hold time
    LCD_WriteReg(0xB600,0x05);
    //Gate EQ control
    LCD_WriteReg(0xB700,0x70);
    LCD_WriteReg(0xB701,0x70);
    //Source EQ control (Mode 2)
    LCD_WriteReg(0xB800,0x01);
    LCD_WriteReg(0xB801,0x03);
    LCD_WriteReg(0xB802,0x03);
    LCD_WriteReg(0xB803,0x03);
    //Inversion mode (2-dot)
    LCD_WriteReg(0xBC00,0x02);
    LCD_WriteReg(0xBC01,0x00);
    LCD_WriteReg(0xBC02,0x00);
    //Timing control 4H w/ 4-delay
    LCD_WriteReg(0xC900,0xD0);
    LCD_WriteReg(0xC901,0x02);
    LCD_WriteReg(0xC902,0x50);
    LCD_WriteReg(0xC903,0x50);
    LCD_WriteReg(0xC904,0x50);
    LCD_WriteReg(0x3500,0x00);
    LCD_WriteReg(0x3A00,0x55);  //16-bit/pixel
    LCD_WR_REG(0x1100);
    opt_delay(120);
    LCD_WR_REG(0x2900);
  }else if(lcddev.id==0X1963)
  {
    LCD_WR_REG(0xE2);		//Set PLL with OSC = 10MHz (hardware),	Multiplier N = 35, 250MHz < VCO < 800MHz = OSC*(N+1), VCO = 300MHz
    LCD_WR_DATA(0x1D);		//����1
    LCD_WR_DATA(0x02);		//����2 Divider M = 2, PLL = 300/(M+1) = 100MHz
    LCD_WR_DATA(0x04);		//����3 Validate M and N values
    opt_delay(100);
    LCD_WR_REG(0xE0);		// Start PLL command
    LCD_WR_DATA(0x01);		// enable PLL
    HAL_Delay(10);
    LCD_WR_REG(0xE0);		// Start PLL command again
    LCD_WR_DATA(0x03);		// now, use PLL output as system clock
    HAL_Delay(12);
    LCD_WR_REG(0x01);		//��λ
    HAL_Delay(10);

    LCD_WR_REG(0xE6);		//��������Ƶ��,33Mhz
    LCD_WR_DATA(0x2F);
    LCD_WR_DATA(0xFF);
    LCD_WR_DATA(0xFF);

    LCD_WR_REG(0xB0);		//����LCDģʽ
    LCD_WR_DATA(0x20);		//24λģʽ
    LCD_WR_DATA(0x00);		//TFT ģʽ

    LCD_WR_DATA((SSD_HOR_RESOLUTION-1)>>8);//����LCDˮƽ����
    LCD_WR_DATA(SSD_HOR_RESOLUTION-1);
    LCD_WR_DATA((SSD_VER_RESOLUTION-1)>>8);//����LCD��ֱ����
    LCD_WR_DATA(SSD_VER_RESOLUTION-1);
    LCD_WR_DATA(0x00);		//RGB����

    LCD_WR_REG(0xB4);		//Set horizontal period
    LCD_WR_DATA((SSD_HT-1)>>8);
    LCD_WR_DATA(SSD_HT-1);
    LCD_WR_DATA(SSD_HPS>>8);
    LCD_WR_DATA(SSD_HPS);
    LCD_WR_DATA(SSD_HOR_PULSE_WIDTH-1);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_REG(0xB6);		//Set vertical period
    LCD_WR_DATA((SSD_VT-1)>>8);
    LCD_WR_DATA(SSD_VT-1);
    LCD_WR_DATA(SSD_VPS>>8);
    LCD_WR_DATA(SSD_VPS);
    LCD_WR_DATA(SSD_VER_FRONT_PORCH-1);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xF0);	//����SSD1963��CPU�ӿ�Ϊ16bit
    LCD_WR_DATA(0x03);	//16-bit(565 format) data for 16bpp

    LCD_WR_REG(0x29);	//������ʾ
    //����PWM���  ����ͨ��ռ�ձȿɵ�
    LCD_WR_REG(0xD0);	//�����Զ���ƽ��DBC
    LCD_WR_DATA(0x00);	//disable

    LCD_WR_REG(0xBE);	//����PWM���
    LCD_WR_DATA(0x05);	//1����PWMƵ��
    LCD_WR_DATA(0xFE);	//2����PWMռ�ձ�
    LCD_WR_DATA(0x01);	//3����C
    LCD_WR_DATA(0x00);	//4����D
    LCD_WR_DATA(0x00);	//5����E
    LCD_WR_DATA(0x00);	//6����F

    LCD_WR_REG(0xB8);	//����GPIO����
    LCD_WR_DATA(0x03);	//2��IO�����ó����
    LCD_WR_DATA(0x01);	//GPIOʹ��������IO����
    LCD_WR_REG(0xBA);
    LCD_WR_DATA(0X01);	//GPIO[1:0]=01,����LCD����

    LCD_SSD_BackLightSet(100);//��������Ϊ����
  }
  //��ʼ������Ժ�,����
  if(lcddev.id==0X9341||lcddev.id==0X5310||lcddev.id==0X5510||lcddev.id==0X1963)//������⼸��IC,������WRʱ��Ϊ���
  {
    //��������дʱ����ƼĴ�����ʱ��
    FSMC_Bank1E->BWTR[6]&=~(0XF<<0);//��ַ����ʱ��(ADDSET)����
    FSMC_Bank1E->BWTR[6]&=~(0XF<<8);//���ݱ���ʱ������
    FSMC_Bank1E->BWTR[6]|=3<<0;		//��ַ����ʱ��(ADDSET)Ϊ3��HCLK =18ns
    FSMC_Bank1E->BWTR[6]|=2<<8; 	//���ݱ���ʱ��(DATAST)Ϊ6ns*3��HCLK=18ns
  }
  LCD_Display_Dir(0);		//Ĭ��Ϊ����
  GPIOB->ODR |= 1<<15;					//��������
  LCD_Clear(WHITE);
}
//��������
//color:Ҫ���������ɫ
void LCD_Clear(uint32_t color)
{
  uint32_t index=0;
  uint32_t totalpoint=lcddev.width;
  totalpoint*=lcddev.height; 			//�õ��ܵ���
  LCD_SetCursor(0x00,0x0000);			//���ù��λ��
  LCD_WriteRAM_Prepare();     		//��ʼд��GRAM
  for(index=0;index<totalpoint;index++)
  {
    TFT_LCD->LCD_RAM=color;
  }
}
//��ָ����������䵥����ɫ
//(sx,sy),(ex,ey):�����ζԽ�����,�����СΪ:(ex-sx+1)*(ey-sy+1)
//color:Ҫ������ɫ
void LCD_Fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint32_t color)
{
  uint16_t i,j;
  uint16_t xlen=0;
  xlen=ex-sx+1;
  for(i=sy;i<=ey;i++)
  {
    LCD_SetCursor(sx,i);      				//���ù��λ��
    LCD_WriteRAM_Prepare();     			//��ʼд��GRAM
    for(j=0;j<xlen;j++)TFT_LCD->LCD_RAM=color;	//��ʾ��ɫ
  }
}
//��ָ�����������ָ����ɫ��
//(sx,sy),(ex,ey):�����ζԽ�����,�����СΪ:(ex-sx+1)*(ey-sy+1)
//color:Ҫ������ɫ
void LCD_Color_Fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t *color)
{
  uint16_t height,width;
  uint16_t i,j;
  width=ex-sx+1; 			//�õ����Ŀ��
  height=ey-sy+1;			//�߶�
  for(i=0;i<height;i++)
  {
    LCD_SetCursor(sx,sy+i);   	//���ù��λ��
    LCD_WriteRAM_Prepare();     //��ʼд��GRAM
    for(j=0;j<width;j++)TFT_LCD->LCD_RAM=color[i*width+j];//д������
  }
}
//����
//x1,y1:�������
//x2,y2:�յ�����
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
  uint16_t t;
  int xerr=0,yerr=0,delta_x,delta_y,distance;
  int incx,incy,uRow,uCol;
  delta_x=x2-x1; //������������
  delta_y=y2-y1;
  uRow=x1;
  uCol=y1;
  if(delta_x>0)incx=1; //���õ�������
  else if(delta_x==0)incx=0;//��ֱ��
  else {incx=-1;delta_x=-delta_x;}
  if(delta_y>0)incy=1;
  else if(delta_y==0)incy=0;//ˮƽ��
  else{incy=-1;delta_y=-delta_y;}
  if( delta_x>delta_y)distance=delta_x; //ѡȡ��������������
  else distance=delta_y;
  for(t=0;t<=distance+1;t++ )//�������
  {
    LCD_DrawPoint(uRow,uCol);//����
    xerr+=delta_x ;
    yerr+=delta_y ;
    if(xerr>distance)
    {
      xerr-=distance;
      uRow+=incx;
    }
    if(yerr>distance)
    {
      yerr-=distance;
      uCol+=incy;
    }
  }
}
//������
//(x1,y1),(x2,y2):���εĶԽ�����
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
  LCD_DrawLine(x1,y1,x2,y1);
  LCD_DrawLine(x1,y1,x1,y2);
  LCD_DrawLine(x1,y2,x2,y2);
  LCD_DrawLine(x2,y1,x2,y2);
}
//��ָ��λ�û�һ��ָ����С��Բ
//(x,y):���ĵ�
//r    :�뾶
void LCD_Draw_Circle(uint16_t x0,uint16_t y0,uint8_t r)
{
  int a,b;
  int di;
  a=0;b=r;
  di=3-(r<<1);             //�ж��¸���λ�õı�־
  while(a<=b)
  {
    LCD_DrawPoint(x0+a,y0-b);             //5
    LCD_DrawPoint(x0+b,y0-a);             //0
    LCD_DrawPoint(x0+b,y0+a);             //4
    LCD_DrawPoint(x0+a,y0+b);             //6
    LCD_DrawPoint(x0-a,y0+b);             //1
    LCD_DrawPoint(x0-b,y0+a);
    LCD_DrawPoint(x0-a,y0-b);             //2
    LCD_DrawPoint(x0-b,y0-a);             //7
    a++;
    //ʹ��Bresenham�㷨��Բ
    if(di<0)di +=4*a+6;
    else
    {
      di+=10+4*(a-b);
      b--;
    }
  }
}
//��ָ��λ����ʾһ���ַ�
//x,y:��ʼ����
//num:Ҫ��ʾ���ַ�:" "--->"~"
//size:�����С 12/16/24/32
//mode:���ӷ�ʽ(1)���Ƿǵ��ӷ�ʽ(0)
void LCD_ShowChar(uint16_t x,uint16_t y,uint8_t num,uint8_t size,uint8_t mode)
{
  uint8_t temp,t1,t;
  uint16_t y0=y;
  uint8_t csize=(size/8+((size%8)?1:0))*(size/2);		//�õ�����һ���ַ���Ӧ������ռ���ֽ���
  num=num-' ';//�õ�ƫ�ƺ��ֵ��ASCII�ֿ��Ǵӿո�ʼȡģ������-' '���Ƕ�Ӧ�ַ����ֿ⣩
  for(t=0;t<csize;t++)
  {
    if(size==12)temp=asc2_1206[num][t]; 	 	//����1206����
    else if(size==16)temp=asc2_1608[num][t];	//����1608����
    else if(size==24)temp=asc2_2412[num][t];	//����2412����
    else if(size==32)temp=asc2_3216[num][t];	//����3216����
    else return;								//û�е��ֿ�
    for(t1=0;t1<8;t1++)
    {
      if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
      else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
      temp<<=1;
      y++;
      if(y>=lcddev.height)return;		//��������
      if((y-y0)==size)
      {
        y=y0;
        x++;
        if(x>=lcddev.width)return;	//��������
        break;
      }
    }
  }
}
//m^n����
//����ֵ:m^n�η�.
uint32_t LCD_Pow(uint8_t m,uint8_t n)
{
  uint32_t result=1;
  while(n--)result*=m;
  return result;
}
//��ʾ����,��λΪ0,����ʾ
//x,y :�������
//len :���ֵ�λ��
//size:�����С
//color:��ɫ
//num:��ֵ(0~4294967295);
void LCD_ShowNum(uint16_t x,uint16_t y,uint32_t num,uint8_t len,uint8_t size)
{
  uint8_t t,temp;
  uint8_t enshow=0;
  for(t=0;t<len;t++)
  {
    temp=(num/LCD_Pow(10,len-t-1))%10;
    if(enshow==0&&t<(len-1))
    {
      if(temp==0)
      {
        LCD_ShowChar(x+(size/2)*t,y,' ',size,0);
        continue;
      }else enshow=1;

    }
    LCD_ShowChar(x+(size/2)*t,y,temp+'0',size,0);
  }
}
//��ʾ����,��λΪ0,������ʾ
//x,y:�������
//num:��ֵ(0~999999999);
//len:����(��Ҫ��ʾ��λ��)
//size:�����С
//mode:
//[7]:0,�����;1,���0.
//[6:1]:����
//[0]:0,�ǵ�����ʾ;1,������ʾ.
void LCD_ShowxNum(uint16_t x,uint16_t y,uint32_t num,uint8_t len,uint8_t size,uint8_t mode)
{
  uint8_t t,temp;
  uint8_t enshow=0;
  for(t=0;t<len;t++)
  {
    temp=(num/LCD_Pow(10,len-t-1))%10;
    if(enshow==0&&t<(len-1))
    {
      if(temp==0)
      {
        if(mode&0X80)LCD_ShowChar(x+(size/2)*t,y,'0',size,mode&0X01);
        else LCD_ShowChar(x+(size/2)*t,y,' ',size,mode&0X01);
        continue;
      }else enshow=1;

    }
    LCD_ShowChar(x+(size/2)*t,y,temp+'0',size,mode&0X01);
  }
}
//��ʾ�ַ���
//x,y:�������
//width,height:�����С
//size:�����С
//*p:�ַ�����ʼ��ַ
void LCD_ShowString(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint8_t size,uint8_t *p)
{
  uint8_t x0=x;
  width+=x;
  height+=y;
  while((*p<='~')&&(*p>=' '))//�ж��ǲ��ǷǷ��ַ�!
  {
    if(x>=width){x=x0;y+=size;}
    if(y>=height)break;//�˳�
    LCD_ShowChar(x,y,*p,size,0);
    x+=size/2;
    p++;
  }
}
#endif
