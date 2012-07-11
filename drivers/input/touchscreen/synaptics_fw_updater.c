/*
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Copyright (c) 2011 Synaptics, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

#include "synaptics_fw.h"	/* FW. data file */
#include "synaptics_fw_updater.h"
#include "synaptics_reg.h"

/* Variables for F34 functionality */
unsigned short SynaF34DataBase;
unsigned short SynaF34QueryBase;
unsigned short SynaF01DataBase;
unsigned short SynaF01CommandBase;
unsigned short SynaF34Reflash_BlockNum;
unsigned short SynaF34Reflash_BlockData;
unsigned short SynaF34ReflashQuery_BootID;
unsigned short SynaF34ReflashQuery_FlashPropertyQuery;
unsigned short SynaF34ReflashQuery_FirmwareBlockSize;
unsigned short SynaF34ReflashQuery_FirmwareBlockCount;
unsigned short SynaF34ReflashQuery_ConfigBlockSize;
unsigned short SynaF34ReflashQuery_ConfigBlockCount;
unsigned short SynaFirmwareBlockSize;
unsigned short SynaFirmwareBlockCount;
unsigned long SynaImageSize;
unsigned short SynaConfigBlockSize;
unsigned short SynaConfigBlockCount;
unsigned long SynaConfigImageSize;
unsigned short SynaBootloadID;
unsigned short SynaF34_FlashControl;
u8 *SynafirmwareImgData;
u8 *SynaconfigImgData;
u8 *SynalockImgData;
unsigned int SynafirmwareImgVersion;

u8 FirmwareImage[16000];
u8 ConfigImage[16000];

static int writeRMI(struct i2c_client *client, u8 addr, u8 *buf, u16 count)
{
	struct i2c_msg msg;
	int ret, i;
	u8 data[256];

	data[0] = addr;

	for (i = 1; i <= count; i++)
		data[i] = *buf++;

	msg.addr = client->addr;
	msg.flags = client->flags & I2C_M_TEN;
	msg.len = count + 1;
	msg.buf = data;

	ret = i2c_transfer(client->adapter, &msg, 1);

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	   transmitted, else error code. */
	return (ret == 1) ? count : ret;
}

static int readRMI(struct i2c_client *client, u8 addr, u8 *buf, u16 count)
{
	struct i2c_msg msg[2];
	int ret;

	msg[0].addr = client->addr;
	msg[0].flags = 0x00;
	msg[0].len = 1;
	msg[0].buf = (u8 *)&addr;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = count;
	msg[1].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 2);

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	   transmitted, else error code. */
	return (ret == 1) ? count : ret;
}

/* SynaSetup scans the Page Description Table (PDT) and sets up the necessary
 * variables for the reflash process. This function is a "slim" version of the
 * PDT scan function in PDT.c, since only F34 and F01 are needed for reflash.
 */
static void SynaSetup(struct synaptics_ts_data *ts)
{
	u8 address;
	u8 buffer[6];

	printk(KERN_DEBUG "[TSP] %s\n", __func__);

	for (address = 0xe9; address > 0xc0; address = address - 6) {
		readRMI(ts->client, address, buffer, 6);

		switch (buffer[5]) {
		case 0x34:
			SynaF34DataBase = buffer[3];
			SynaF34QueryBase = buffer[0];
			break;

		case 0x01:
			SynaF01DataBase = buffer[3];
			SynaF01CommandBase = buffer[1];
			break;
		}
	}

	printk(KERN_DEBUG "[TSP] SynaF34_FlashControl : %u\n",
			SynaF34_FlashControl);

	SynaF34Reflash_BlockNum = SynaF34DataBase;
	SynaF34Reflash_BlockData = SynaF34DataBase + 2;
	SynaF34ReflashQuery_BootID = SynaF34QueryBase;
	SynaF34ReflashQuery_FlashPropertyQuery = SynaF34QueryBase + 2;
	SynaF34ReflashQuery_FirmwareBlockSize = SynaF34QueryBase + 3;
	SynaF34ReflashQuery_FirmwareBlockCount = SynaF34QueryBase + 5;
	SynaF34ReflashQuery_ConfigBlockSize = SynaF34QueryBase + 3;
	SynaF34ReflashQuery_ConfigBlockCount = SynaF34QueryBase + 7;

	SynafirmwareImgData = (u8 *)((&SynaFirmware[0]) + 0x100);
	SynaconfigImgData = (u8 *)(SynafirmwareImgData + SynaImageSize);
	SynafirmwareImgVersion = (u32)(SynaFirmware[7]);

	switch (SynafirmwareImgVersion) {
	case 2:
		SynalockImgData = (u8 *)((&SynaFirmware[0]) + 0xD0);
		break;
	case 3:
	case 4:
		SynalockImgData = (u8 *)((&SynaFirmware[0]) + 0xC0);
		break;
	case 5:
		SynalockImgData = (u8 *)((&SynaFirmware[0]) + 0xB0);
		break;
	default:
		break;
	}
}

/* SynaInitialize sets up the reflahs process
 */
static void SynaInitialize(struct synaptics_ts_data *ts)
{
	u8 uData[2];

	printk(KERN_DEBUG "[TSP] %s\n", __func__);

	uData[0] = 0x00;
	writeRMI(ts->client, 0xff, uData, 1);

	SynaSetup(ts);

	SynafirmwareImgData = &FirmwareImage[0];
	SynaconfigImgData = &ConfigImage[0];

	readRMI(ts->client, SynaF34ReflashQuery_FirmwareBlockSize, uData, 2);

	SynaFirmwareBlockSize = uData[0] | (uData[1] << 8);

}

/* SynaReadFirmwareInfo reads the F34 query registers and retrieves the block
 * size and count of the firmware section of the image to be reflashed
 */
static void SynaReadFirmwareInfo(struct synaptics_ts_data *ts)
{
	u8 uData[2];
	printk(KERN_DEBUG "[TSP] %s\n", __func__);

	readRMI(ts->client, SynaF34ReflashQuery_FirmwareBlockSize, uData, 2);
	SynaFirmwareBlockSize = uData[0] | (uData[1] << 8);

	readRMI(ts->client, SynaF34ReflashQuery_FirmwareBlockCount, uData, 2);
	SynaFirmwareBlockCount = uData[0] | (uData[1] << 8);
	SynaImageSize = SynaFirmwareBlockCount * SynaFirmwareBlockSize;
}

/* SynaReadConfigInfo reads the F34 query registers and retrieves the block size
 * and count of the configuration section of the image to be reflashed
 */
static void SynaReadConfigInfo(struct synaptics_ts_data *ts)
{
	u8 uData[2];

	printk(KERN_DEBUG "[TSP] %s\n", __func__);

	readRMI(ts->client, SynaF34ReflashQuery_ConfigBlockSize, uData, 2);
	SynaConfigBlockSize = uData[0] | (uData[1] << 8);

	readRMI(ts->client, SynaF34ReflashQuery_ConfigBlockCount, uData, 2);
	SynaConfigBlockCount = uData[0] | (uData[1] << 8);
	SynaConfigImageSize = SynaConfigBlockCount * SynaConfigBlockSize;
}

/* SynaReadBootloadID reads the F34 query registers and retrieves the bootloader
 * ID of the firmware
 */
static void SynaReadBootloadID(struct synaptics_ts_data *ts)
{
	u8 uData[2];
	printk(KERN_DEBUG "[TSP] %s\n", __func__);

	readRMI(ts->client, SynaF34ReflashQuery_BootID, uData, 2);
	SynaBootloadID = uData[0] + uData[1] * 0x100;
	printk(KERN_DEBUG "[TSP] SynaBootloadID : %u\n", SynaBootloadID);
}

/* SynaWriteBootloadID writes the bootloader ID to the F34 data register to
 * unlock the reflash process
 */
static void SynaWriteBootloadID(struct synaptics_ts_data *ts)
{
	u8 uData[2];
	printk(KERN_DEBUG "[TSP] %s\n", __func__);

	uData[0] = SynaBootloadID % 0x100;
	uData[1] = SynaBootloadID / 0x100;

	writeRMI(ts->client, SynaF34Reflash_BlockData, uData, 2);
}

/* SynaEnableFlashing kicks off the reflash process
 */
static void SynaEnableFlashing(struct synaptics_ts_data *ts)
{
	u8 uData;
	u8 uStatus;

	printk(KERN_DEBUG "[TSP] %s\n", __func__);

	/* Reflash is enabled by first reading the bootloader ID from
	   the firmware and write it back */
	SynaReadBootloadID(ts);
	SynaWriteBootloadID(ts);

	/* Make sure Reflash is not already enabled */
	do {
		readRMI(ts->client, SynaF34_FlashControl, &uData, 1);
	} while (((uData & 0x0f) != 0x00));

	readRMI(ts->client, SynaF01DataBase, &uStatus, 1);

	if ((uStatus & 0x40) == 0) {
		/* Write the "Enable Flash Programming command to
		F34 Control register Wait for ATTN and then clear the ATTN. */
		uData = 0x0f;
		writeRMI(ts->client, SynaF34_FlashControl, &uData, 1);
		mdelay(300);
		readRMI(ts->client, (SynaF01DataBase + 1), &uStatus, 1);

		/* Scan the PDT again to ensure all register offsets are
		correct */
		SynaSetup(ts);

		/* Read the "Program Enabled" bit of the F34 Control register,
		and proceed only if the bit is set.*/
		readRMI(ts->client, SynaF34_FlashControl, &uData, 1);

		while (uData != 0x80) {
			/* In practice, if uData!=0x80 happens for multiple
			counts, it indicates reflash is failed to be enabled,
			and program should quit */
			;
		}
	}
}

/* SynaWaitATTN waits for ATTN to be asserted within a certain time threshold.
 * The function also checks for the F34 "Program Enabled" bit and clear ATTN
 * accordingly.
 */
static void SynaWaitATTN(struct synaptics_ts_data *ts)
{
	u8 uData;
	u8 uStatus;
	int cnt = 1;

	while (gpio_get_value(GPIO_TSP_INT) && cnt < 300) {
		mdelay(1);
		cnt++;
	}
	do {
		readRMI(ts->client, SynaF34_FlashControl, &uData, 1);
		readRMI(ts->client, (SynaF01DataBase + 1), &uStatus, 1);
	} while (uData != 0x80);
}

/* SynaProgramConfiguration writes the configuration section of the image block
 * by block
 */
static void SynaProgramConfiguration(struct synaptics_ts_data *ts)
{
	u8 uData[2];
	u8 *puData;
	unsigned short blockNum;

	puData = (u8 *) &SynaFirmware[0xb100];

	printk(KERN_DEBUG "[TSP] Program Configuration Section...\n");

	for (blockNum = 0; blockNum < SynaConfigBlockCount; blockNum++) {
		uData[0] = blockNum & 0xff;
		uData[1] = (blockNum & 0xff00) >> 8;

		/* Block by blcok, write the block number and data to
		the corresponding F34 data registers */
		writeRMI(ts->client, SynaF34Reflash_BlockNum, uData, 2);
		writeRMI(ts->client, SynaF34Reflash_BlockData,
			puData, SynaConfigBlockSize);
		puData += SynaConfigBlockSize;

		/* Issue the "Write Configuration Block" command */
		uData[0] = 0x06;
		writeRMI(ts->client, SynaF34_FlashControl, uData, 1);
		SynaWaitATTN(ts);
		printk(KERN_DEBUG ".");
	}
}

/* SynaFinalizeReflash finalizes the reflash process
*/
static void SynaFinalizeReflash(struct synaptics_ts_data *ts)
{
	u8 uData;
	u8 uStatus;

	printk(KERN_DEBUG "[TSP] Finalizing Reflash..\n");

	/* Issue the "Reset" command to F01 command register to reset the chip
	 This command will also test the new firmware image and check if its is
	 valid */
	uData = 1;
	writeRMI(ts->client, SynaF01CommandBase, &uData, 1);

	mdelay(300);
	readRMI(ts->client, SynaF01DataBase, &uData, 1);

	/* Sanity check that the reflash process is still enabled */
	do {
		readRMI(ts->client, SynaF34_FlashControl, &uStatus, 1);
	} while ((uStatus & 0x0f) != 0x00);
	readRMI(ts->client, (SynaF01DataBase + 1), &uStatus, 1);

	SynaSetup(ts);

	uData = 0;

	/* Check if the "Program Enabled" bit in F01 data register is cleared
	 Reflash is completed, and the image passes testing when the bit is
	 cleared */
	do {
		readRMI(ts->client, SynaF01DataBase, &uData, 1);
	} while ((uData & 0x40) != 0);

	/* Rescan PDT the update any changed register offsets */
	SynaSetup(ts);

	printk(KERN_DEBUG "[TSP] Reflash Completed. Please reboot.\n");
}

/* SynaFlashFirmwareWrite writes the firmware section of the image block by
 * block
 */
static void SynaFlashFirmwareWrite(struct synaptics_ts_data *ts)
{
	u8 *puFirmwareData;
	u8 uData[2];
	unsigned short blockNum;

	printk(KERN_DEBUG "[TSP] %s\n", __func__);

	puFirmwareData = (u8 *) &SynaFirmware[0x100];

	for (blockNum = 0; blockNum < SynaFirmwareBlockCount; ++blockNum) {
		/* Block by blcok, write the block number and data to
		the corresponding F34 data registers */
		uData[0] = blockNum & 0xff;
		uData[1] = (blockNum & 0xff00) >> 8;
		writeRMI(ts->client, SynaF34Reflash_BlockNum, uData, 2);

		writeRMI(ts->client, SynaF34Reflash_BlockData, puFirmwareData,
			SynaFirmwareBlockSize);
		puFirmwareData += SynaFirmwareBlockSize;

		/* Issue the "Write Firmware Block" command */
		uData[0] = 2;
		writeRMI(ts->client, SynaF34_FlashControl, uData, 1);

		SynaWaitATTN(ts);
	}
}

/* SynaProgramFirmware prepares the firmware writing process
*/
static void SynaProgramFirmware(struct synaptics_ts_data *ts)
{
	u8 uData;

	printk(KERN_DEBUG "[TSP] %s\n", __func__);

	SynaReadBootloadID(ts);
	SynaWriteBootloadID(ts);

	uData = 3;
	writeRMI(ts->client, SynaF34_FlashControl, &uData, 1);

	SynaWaitATTN(ts);
	SynaFlashFirmwareWrite(ts);
}

/* eraseConfigBlock erases the config block
*/
static void eraseConfigBlock(struct synaptics_ts_data *ts)
{
	u8 uData;

	printk(KERN_DEBUG "[TSP] %s\n", __func__);
	/* Erase of config block is done by first entering into
	bootloader mode */
	SynaReadBootloadID(ts);
	SynaWriteBootloadID(ts);

	/* Command 7 to erase config block */
	uData = 7;
	writeRMI(ts->client, SynaF34_FlashControl, &uData, 1);

	SynaWaitATTN(ts);
}

static u8 get_fw_version_address(struct synaptics_ts_data *ts)
{
	u8 ret = 0;
	u8 address;
	u8 buffer[6];

	for (address = 0xe9; address > 0xd0; address -= 6) {
		readRMI(ts->client, address, buffer, 6);

		if (!buffer[5])
			break;
		switch (buffer[5]) {
		case 0x34:
			ret = buffer[2];
			break;
		}
	}

	return ret;
}

void synaptics_fw_updater(struct synaptics_ts_data *ts)
{
	u8 buf[4];
	u8 fw_version[4] = {SynaFirmware[0xb100], SynaFirmware[0xb101],
				SynaFirmware[0xb102], SynaFirmware[0xb103]};
	u8 addr = get_fw_version_address(ts);

	readRMI(ts->client, addr, buf, 4);
	printk(KERN_DEBUG "[TSP] IC FW. : [%s], new FW. : [%s]\n",
		(char *)buf, (char *)fw_version);

	if (strncmp((char *)fw_version, (char *)buf, 4) != 0) {
		SynaInitialize(ts);
		SynaReadConfigInfo(ts);
		SynaReadFirmwareInfo(ts);
		SynaF34_FlashControl = SynaF34DataBase
			+ SynaFirmwareBlockSize + 2;
		printk(KERN_DEBUG "[TSP] SynaF34_FlashControl : %u\n",
			SynaF34_FlashControl);
		SynaEnableFlashing(ts);
		SynaProgramFirmware(ts);
		SynaProgramConfiguration(ts);
		SynaFinalizeReflash(ts);
	}
}
