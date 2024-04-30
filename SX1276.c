/**
 * Author C J R Y U | 2sanghaesea
 *
 * work based on
 * https://github.com/wdomski/SX1278/
 */

#include <string.h>
#include <SX1276.h>

uint8_t SX1276_SPIRead(SX1276_t *module, uint8_t addr) {
	uint8_t tmp;
	SX1276_hw_SPICommand(module->hw, addr);
	tmp = SX1276_hw_SPIReadByte(module->hw);
	SX1276_hw_SetNSS(module->hw, 1);
	return tmp;
}

void SX1276_SPIWrite(SX1276_t *module, uint8_t addr, uint8_t cmd) {
	SX1276_hw_SetNSS(module->hw, 0);
	SX1276_hw_SPICommand(module->hw, addr | 0x80);
	SX1276_hw_SPICommand(module->hw, cmd);
	SX1276_hw_SetNSS(module->hw, 1);
}

void SX1276_SPIBurstRead(SX1276_t *module, uint8_t addr, uint8_t *rxBuf,
		uint8_t length) {
	uint8_t i;
	if (length <= 1) {
		return;
	} else {
		SX1276_hw_SetNSS(module->hw, 0);
		SX1276_hw_SPICommand(module->hw, addr);
		for (i = 0; i < length; i++) {
			*(rxBuf + i) = SX1276_hw_SPIReadByte(module->hw);
		}
		SX1276_hw_SetNSS(module->hw, 1);
	}
}

void SX1276_SPIBurstWrite(SX1276_t *module, uint8_t addr, uint8_t *txBuf,
		uint8_t length) {
	unsigned char i;
	if (length <= 1) {
		return;
	} else {
		SX1276_hw_SetNSS(module->hw, 0);
		SX1276_hw_SPICommand(module->hw, addr | 0x80);
		for (i = 0; i < length; i++) {
			SX1276_hw_SPICommand(module->hw, *(txBuf + i));
		}
		SX1276_hw_SetNSS(module->hw, 1);
	}
}

void SX1276_config(SX1276_t *module) {
	SX1276_sleep(module); //Change modem mode Must in Sleep mode
	SX1276_hw_DelayMs(15);

	SX1276_entryLoRa(module);
	//SX1276_SPIWrite(module, 0x5904); //?? Change digital regulator form 1.6V to 1.47V: see errata note

	uint64_t freq = ((uint64_t) module->frequency << 19) / 32000000;
	uint8_t freq_reg[3];
	freq_reg[0] = (uint8_t) (freq >> 16);
	freq_reg[1] = (uint8_t) (freq >> 8);
	freq_reg[2] = (uint8_t) (freq >> 0);
	SX1276_SPIBurstWrite(module, LR_RegFrMsb, (uint8_t*) freq_reg, 3); //setting  frequency parameter

	SX1276_SPIWrite(module, RegSyncWord, 0x34);

	//setting base parameter
	SX1276_SPIWrite(module, LR_RegPaConfig, SX1276_Power[module->power]); //Setting output power parameter

	SX1276_SPIWrite(module, LR_RegOcp, 0x0B);			//RegOcp,Close Ocp
	SX1276_SPIWrite(module, LR_RegLna, 0x23);		//RegLNA,High & LNA Enable
	if (SX1276_SpreadFactor[module->LoRa_SF] == 6) {	//SFactor=6
		uint8_t tmp;
		SX1276_SPIWrite(module,
		LR_RegModemConfig1,
				((SX1276_LoRaBandwidth[module->LoRa_BW] << 4)
						+ (SX1276_CodingRate[module->LoRa_CR] << 1) + 0x01)); //Implicit Enable CRC Enable(0x02) & Error Coding rate 4/5(0x01), 4/6(0x02), 4/7(0x03), 4/8(0x04)

		SX1276_SPIWrite(module,
		LR_RegModemConfig2,
				((SX1276_SpreadFactor[module->LoRa_SF] << 4)
						+ (SX1276_CRC_Sum[module->LoRa_CRC_sum] << 2) + 0x03));

		tmp = SX1276_SPIRead(module, 0x31);
		tmp &= 0xF8;
		tmp |= 0x05;
		SX1276_SPIWrite(module, 0x31, tmp);
		SX1276_SPIWrite(module, 0x37, 0x0C);
	} else {
		SX1276_SPIWrite(module,
		LR_RegModemConfig1,
				((SX1276_LoRaBandwidth[module->LoRa_BW] << 4)
						+ (SX1276_CodingRate[module->LoRa_CR] << 1) + 0x00)); //Explicit Enable CRC Enable(0x02) & Error Coding rate 4/5(0x01), 4/6(0x02), 4/7(0x03), 4/8(0x04)

		SX1276_SPIWrite(module,
		LR_RegModemConfig2,
				((SX1276_SpreadFactor[module->LoRa_SF] << 4)
						+ (SX1276_CRC_Sum[module->LoRa_CRC_sum] << 2) + 0x00)); //SFactor &  LNA gain set by the internal AGC loop
	}

	SX1276_SPIWrite(module, LR_RegModemConfig3, 0x04);
	SX1276_SPIWrite(module, LR_RegSymbTimeoutLsb, 0x08); //RegSymbTimeoutLsb Timeout = 0x3FF(Max)
	SX1276_SPIWrite(module, LR_RegPreambleMsb, 0x00); //RegPreambleMsb
	SX1276_SPIWrite(module, LR_RegPreambleLsb, 8); //RegPreambleLsb 8+4=12byte Preamble
	SX1276_SPIWrite(module, REG_LR_DIOMAPPING2, 0x01); //RegDioMapping2 DIO5=00, DIO4=01
	module->readBytes = 0;
	SX1276_standby(module); //Entry standby mode
}

void SX1276_standby(SX1276_t *module) {
	SX1276_SPIWrite(module, LR_RegOpMode, 0x09);
	module->status = STANDBY;
}

void SX1276_sleep(SX1276_t *module) {
	SX1276_SPIWrite(module, LR_RegOpMode, 0x08);
	module->status = SLEEP;
}

void SX1276_entryLoRa(SX1276_t *module) {
	SX1276_SPIWrite(module, LR_RegOpMode, 0x88);
}

void SX1276_clearLoRaIrq(SX1276_t *module) {
	SX1276_SPIWrite(module, LR_RegIrqFlags, 0xFF);
}

int SX1276_LoRaEntryRx(SX1276_t *module, uint8_t length, uint32_t timeout) {
	uint8_t addr;

	module->packetLength = length;

	SX1276_config(module);		//Setting base parameter
	SX1276_SPIWrite(module, REG_LR_PADAC, 0x84);	//Normal and RX
	SX1276_SPIWrite(module, LR_RegHopPeriod, 0xFF);	//No FHSS
	SX1276_SPIWrite(module, REG_LR_DIOMAPPING1, 0x01);//DIO=00,DIO1=00,DIO2=00, DIO3=01
	SX1276_SPIWrite(module, LR_RegIrqFlagsMask, 0x3F);//Open RxDone interrupt & Timeout
	SX1276_clearLoRaIrq(module);
	SX1276_SPIWrite(module, LR_RegPayloadLength, length);//Payload Length 21byte(this register must difine when the data long of one byte in SF is 6)
	addr = SX1276_SPIRead(module, LR_RegFifoRxBaseAddr); //Read RxBaseAddr
	SX1276_SPIWrite(module, LR_RegFifoAddrPtr, addr); //RxBaseAddr->FiFoAddrPtr
	SX1276_SPIWrite(module, LR_RegOpMode, 0x8d);	//Mode//Low Frequency Mode
	//SX1276_SPIWrite(module, LR_RegOpMode,0x05);	//Continuous Rx Mode //High Frequency Mode
	module->readBytes = 0;

	while (1) {
		if ((SX1276_SPIRead(module, LR_RegModemStat) & 0x04) == 0x04) {	//Rx-on going RegModemStat
			module->status = RX;
			return 1;
		}
		if (--timeout == 0) {
			SX1276_hw_Reset(module->hw);
			SX1276_config(module);
			return 0;
		}
		SX1276_hw_DelayMs(1);
	}
}

uint8_t SX1276_LoRaRxPacket(SX1276_t *module) {
	unsigned char addr;
	unsigned char packet_size;

	if (SX1276_hw_GetDIO0(module->hw)) {
		memset(module->rxBuffer, 0x00, SX1276_MAX_PACKET);

		addr = SX1276_SPIRead(module, LR_RegFifoRxCurrentaddr); //last packet addr
		SX1276_SPIWrite(module, LR_RegFifoAddrPtr, addr); //RxBaseAddr -> FiFoAddrPtr

		if (module->LoRa_SF == SX1276_LORA_SF_6) { //When SpreadFactor is six,will used Implicit Header mode(Excluding internal packet length)
			packet_size = module->packetLength;
		} else {
			packet_size = SX1276_SPIRead(module, LR_RegRxNbBytes); //Number for received bytes
		}

		SX1276_SPIBurstRead(module, 0x00, module->rxBuffer, packet_size);
		module->readBytes = packet_size;
		SX1276_clearLoRaIrq(module);
	}
	return module->readBytes;
}

int SX1276_LoRaEntryTx(SX1276_t *module, uint8_t length, uint32_t timeout) {
	uint8_t addr;
	uint8_t temp;

	module->packetLength = length;

	SX1276_config(module); //setting base parameter
	SX1276_SPIWrite(module, REG_LR_PADAC, 0x87);	//Tx for 20dBm
	SX1276_SPIWrite(module, LR_RegHopPeriod, 0x00); //RegHopPeriod NO FHSS
	SX1276_SPIWrite(module, REG_LR_DIOMAPPING1, 0x41); //DIO0=01, DIO1=00,DIO2=00, DIO3=01
	SX1276_clearLoRaIrq(module);
	SX1276_SPIWrite(module, LR_RegIrqFlagsMask, 0xF7); //Open TxDone interrupt
	SX1276_SPIWrite(module, LR_RegPayloadLength, length); //RegPayloadLength 21byte
	addr = SX1276_SPIRead(module, LR_RegFifoTxBaseAddr); //RegFiFoTxBaseAddr
	SX1276_SPIWrite(module, LR_RegFifoAddrPtr, addr); //RegFifoAddrPtr

	while (1) {
		temp = SX1276_SPIRead(module, LR_RegPayloadLength);
		if (temp == length) {
			module->status = TX;
			return 1;
		}

		if (--timeout == 0) {
			SX1276_hw_Reset(module->hw);
			SX1276_config(module);
			return 0;
		}
	}
}

int SX1276_LoRaTxPacket(SX1276_t *module, uint8_t *txBuffer, uint8_t length,
		uint32_t timeout) {
	SX1276_SPIBurstWrite(module, 0x00, txBuffer, length);
	SX1276_SPIWrite(module, LR_RegOpMode, 0x8b);	//Tx Mode
	while (1) {
		if (SX1276_hw_GetDIO0(module->hw)) { //if(Get_NIRQ()) //Packet send over
			SX1276_SPIRead(module, LR_RegIrqFlags);
			SX1276_clearLoRaIrq(module); //Clear irq
			SX1276_standby(module); //Entry Standby mode
			return 1;
		}

		if (--timeout == 0) {
			SX1276_hw_Reset(module->hw);
			SX1276_config(module);
			return 0;
		}
		SX1276_hw_DelayMs(1);
	}
}

void SX1276_init(SX1276_t *module, uint64_t frequency, uint8_t power,
		uint8_t LoRa_SF, uint8_t LoRa_BW, uint8_t LoRa_CR,
		uint8_t LoRa_CRC_sum, uint8_t packetLength) {
	SX1276_hw_init(module->hw);
	module->frequency = frequency;
	module->power = power;
	module->LoRa_SF = LoRa_SF;
	module->LoRa_BW = LoRa_BW;
	module->LoRa_CR = LoRa_CR;
	module->LoRa_CRC_sum = LoRa_CRC_sum;
	module->packetLength = packetLength;
	SX1276_config(module);
}

int SX1276_transmit(SX1276_t *module, uint8_t *txBuf, uint8_t length,
		uint32_t timeout) {
	if (SX1276_LoRaEntryTx(module, length, timeout)) {
		return SX1276_LoRaTxPacket(module, txBuf, length, timeout);
	}
	return 0;
}

int SX1276_receive(SX1276_t *module, uint8_t length, uint32_t timeout) {
	return SX1276_LoRaEntryRx(module, length, timeout);
}

uint8_t SX1276_available(SX1276_t *module) {
	return SX1276_LoRaRxPacket(module);
}

uint8_t SX1276_read(SX1276_t *module, uint8_t *rxBuf, uint8_t length) {
	if (length != module->readBytes)
		length = module->readBytes;
	memcpy(rxBuf, module->rxBuffer, length);
	rxBuf[length] = '\0';
	module->readBytes = 0;
	return length;
}

uint8_t SX1276_RSSI_LoRa(SX1276_t *module) {
	uint32_t temp = 10;
	temp = SX1276_SPIRead(module, LR_RegRssiValue); //Read RegRssiValue, Rssi value
	temp = temp + 127 - 137; //127:Max RSSI, 137:RSSI offset
	return (uint8_t) temp;
}

uint8_t SX1276_RSSI(SX1276_t *module) {
	uint8_t temp = 0xff;
	temp = SX1276_SPIRead(module, RegRssiValue);
	temp = 127 - (temp >> 1);	//127:Max RSSI
	return temp;
}
