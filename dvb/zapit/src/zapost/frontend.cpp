/*
 * $Id: frontend.cpp,v 1.53 2003/11/27 00:02:56 obi Exp $
 *
 * (C) 2002-2003 Andreas Oberritter <obi@tuxbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* system c */
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>

/* zapit */
#include <zapit/debug.h>
#include <zapit/frontend.h>
#include <zapit/settings.h>

#define diff(x,y)	(max(x,y) - min(x,y))
#define min(x,y)	((x < y) ? x : y)
#define max(x,y)	((x > y) ? x : y)

CFrontend::CFrontend(void)
{
	tuned = false;
	currentToneMode = SEC_TONE_OFF;
	currentVoltage = SEC_VOLTAGE_13;
	currentFrequency = 0;
	diseqcRepeats = 0;
	diseqcType = NO_DISEQC;

	if ((fd = open(FRONTEND_DEVICE, O_RDWR|O_NONBLOCK)) < 0)
		ERROR(FRONTEND_DEVICE);

	fop(ioctl, FE_GET_INFO, &info);
}

CFrontend::~CFrontend(void)
{
	if (diseqcType > MINI_DISEQC)
		sendDiseqcStandby();
	
	close(fd);
}

fe_code_rate_t CFrontend::getCodeRate(const uint8_t fec_inner)
{
	switch (fec_inner & 0x0F) {
	case 0x01:
		return FEC_1_2;
	case 0x02:
		return FEC_2_3;
	case 0x03:
		return FEC_3_4;
	case 0x04:
		return FEC_5_6;
	case 0x05:
		return FEC_7_8;
	case 0x0F:
		return FEC_NONE;
	default:
		return FEC_AUTO;
	}
}

fe_modulation_t CFrontend::getModulation(const uint8_t modulation)
{
	switch (modulation) {
	case 0x01:
		return QAM_16;
	case 0x02:
		return QAM_32;
	case 0x03:
		return QAM_64;
	case 0x04:
		return QAM_128;
	case 0x05:
		return QAM_256;
	default:
		return QAM_AUTO;
	}
}

uint32_t CFrontend::getFrequency(void) const
{
	switch (info.type) {
	case FE_QPSK:
		if (currentToneMode == SEC_TONE_OFF)
			return currentFrequency + lnbOffsetsLow[currentDiseqc];
		else
			return currentFrequency + lnbOffsetsHigh[currentDiseqc];

	case FE_QAM:
	case FE_OFDM:
	default:
		return currentFrequency;
	}
}

uint8_t CFrontend::getPolarization(void) const
{
	if (currentVoltage == SEC_VOLTAGE_13)
		return 1;
	else
		return 0;
}

fe_status_t CFrontend::getStatus(void) const
{
	fe_status_t status;

	fop(ioctl, FE_READ_STATUS, &status);

	return status;
}

uint32_t CFrontend::getBitErrorRate(void) const
{
	uint32_t ber;

	fop(ioctl, FE_READ_BER, &ber);

	return ber;
}

uint16_t CFrontend::getSignalStrength(void) const
{
	uint16_t strength;

	fop(ioctl, FE_READ_SIGNAL_STRENGTH, &strength);

	return strength;
}

uint16_t CFrontend::getSignalNoiseRatio(void) const
{
	uint16_t snr;

	fop(ioctl, FE_READ_SNR, &snr);

	return snr;
}

uint32_t CFrontend::getUncorrectedBlocks(void) const
{
	uint32_t blocks;

	fop(ioctl, FE_READ_UNCORRECTED_BLOCKS, &blocks);

	return blocks;
}

void CFrontend::setFrontend(const struct dvb_frontend_parameters *feparams)
{
	struct dvb_frontend_event event;

	if (fd == -1)
		return;

	if (errno != 0)
		errno = 0;

	while ((errno == 0) || (errno == EOVERFLOW))
		quiet_fop(ioctl, FE_GET_EVENT, &event);

	fop(ioctl, FE_SET_FRONTEND, feparams);
}

struct dvb_frontend_parameters CFrontend::getFrontend(void) const
{
	struct dvb_frontend_parameters feparams;

	fop(ioctl, FE_GET_FRONTEND, &feparams);

	return feparams;
}

#define TIMEOUT_MAX_MS 3000

struct dvb_frontend_event CFrontend::getEvent(void)
{
	struct dvb_frontend_event event;
	struct pollfd pfd;
	struct timeval tv, tv2;

	int msec = TIMEOUT_MAX_MS;

	pfd.fd = fd;
	pfd.events = POLLIN;

	gettimeofday(&tv, NULL);

	while (msec > 0) {

		int res = poll(&pfd, 1, msec);

		if (res <= 0)
			break;

		gettimeofday(&tv2, NULL);
		msec = TIMEOUT_MAX_MS - ((tv2.tv_sec - tv.tv_sec) * 1000) - ((tv2.tv_usec - tv.tv_usec) / 1000);

		if (pfd.revents & POLLIN) {
#if 0
			/* ignore too fast events - tuning in 0 msec is not possible */
			if ((TIMEOUT_MAX_MS - msec) == 0)
				continue;
#endif
			INFO("event after %d milliseconds", TIMEOUT_MAX_MS - msec);

			memset(&event, 0, sizeof(struct dvb_frontend_event));
	
			fop(ioctl, FE_GET_EVENT, &event);

			if (event.status & FE_HAS_LOCK) {
				currentFrequency = event.parameters.frequency;
				INFO("FE_HAS_LOCK: freq %u", currentFrequency);
				tuned = true;
				break;
			}

			else if ((TIMEOUT_MAX_MS - msec > 1000) && (event.status & FE_TIMEDOUT)) {
				WARN("FE_TIMEDOUT");
				break;
			}

			else {
				if (event.status & FE_HAS_SIGNAL)
					WARN("FE_HAS_SIGNAL");
				if (event.status & FE_HAS_CARRIER)
					WARN("FE_HAS_CARRIER");
				if (event.status & FE_HAS_VITERBI)
					WARN("FE_HAS_VITERBI");
				if (event.status & FE_HAS_SYNC)
					WARN("FE_HAS_SYNC");
				if (event.status & FE_TIMEDOUT)
					WARN("FE_TIMEDOUT");
				if (event.status & FE_REINIT)
					WARN("FE_REINIT");
			}
		}

		else {
			WARN("pfd[0].revents %d", pfd.revents);
		}

	}

	if (!tuned)
		currentFrequency = 0;

	return event;
}

#ifdef DEBUG_SEC_TIMING

#define TIMER_START()									\
	struct timeval tv, tv2;								\
	unsigned int msec;								\
	gettimeofday(&tv, NULL)

#define TIMER_STOP()									\
	gettimeofday(&tv2, NULL);							\
	msec = ((tv2.tv_sec - tv.tv_sec) * 1000) + ((tv2.tv_usec - tv.tv_usec) / 1000); \
	INFO("%u msec", msec)

#else /* DEBUG_SEC_TIMING */

#define TIMER_START() \
	do {} while (0)

#define TIMER_STOP() \
	do {} while (0)

#endif /* DEBUG_SEC_TIMING */

void CFrontend::secSetTone(const fe_sec_tone_mode_t toneMode, const uint32_t ms)
{
	TIMER_START();

	if (fop(ioctl, FE_SET_TONE, toneMode) == 0) {
		currentToneMode = toneMode;
		usleep(1000 * ms);
	}

	TIMER_STOP();
}

void CFrontend::secSetVoltage(const fe_sec_voltage_t voltage, const uint32_t ms)
{
	TIMER_START();
	
	if (fop(ioctl, FE_SET_VOLTAGE, voltage) == 0) {
		currentVoltage = voltage;
		usleep(1000 * ms);
	}
	
	TIMER_STOP();
}

void CFrontend::secResetOverload(void)
{
	TIMER_START();

	fop(ioctl, FE_DISEQC_RESET_OVERLOAD);

	TIMER_STOP();
}

void CFrontend::sendDiseqcCommand(const struct dvb_diseqc_master_cmd *cmd, const uint32_t ms)
{
	TIMER_START();

	if (fop(ioctl, FE_DISEQC_SEND_MASTER_CMD, cmd) == 0)
		usleep(1000 * ms);

	TIMER_STOP();
}

uint32_t CFrontend::getDiseqcReply(const int timeout_ms) const
{
	struct dvb_diseqc_slave_reply reply;

	reply.timeout = timeout_ms;

	TIMER_START();

	if (fop(ioctl, FE_DISEQC_RECV_SLAVE_REPLY, &reply) < 0)
		return 0;

	TIMER_STOP();

	/* timeout */
	if (reply.msg_len == 0)
		return 1;

	switch (reply.msg[0]) {
	case 0xe4:	/* ok */
		return 0;
	case 0xe5:	/* invalid address or unknown command */
		return 1;
	case 0xe6:	/* parity error */
		return 1;
	case 0xe7:	/* contention flag mismatch */
		return 1;
	default:	/* unexpected reply */
		return 0;
	}
}

void CFrontend::sendToneBurst(const fe_sec_mini_cmd_t burst, const uint32_t ms)
{
	TIMER_START();

	if (fop(ioctl, FE_DISEQC_SEND_BURST, burst) == 0)
		usleep(1000 * ms);

	TIMER_STOP();
}

void CFrontend::setDiseqcType(const diseqc_t newDiseqcType)
{
	switch (newDiseqcType) {
	case NO_DISEQC:
		INFO("NO_DISEQC");
		break;
	case MINI_DISEQC:
		INFO("MINI_DISEQC");
		break;
	case SMATV_REMOTE_TUNING:
		INFO("SMATV_REMOTE_TUNING");
		break;
	case DISEQC_1_0:
		INFO("DISEQC_1_0");
		break;
	case DISEQC_1_1:
		INFO("DISEQC_1_1");
		break;
	case DISEQC_1_2:
		INFO("DISEQC_1_2");
		break;
	case DISEQC_2_0:
		INFO("DISEQC_2_0");
		break;
	case DISEQC_2_1:
		INFO("DISEQC_2_1");
		break;
	case DISEQC_2_2:
		INFO("DISEQC_2_2");
		break;
	default:
		WARN("Invalid DiSEqC type");
		return;
	}

	if ((diseqcType <= MINI_DISEQC) && (newDiseqcType > MINI_DISEQC)) {
		sendDiseqcPowerOn();
		sendDiseqcReset();
	}

	diseqcType = newDiseqcType;
}

void CFrontend::setLnbOffset(const bool high, const uint8_t index, int offset)
{
	if (index < MAX_LNBS) {
		if (high)
			lnbOffsetsHigh[index] = offset;
		else
			lnbOffsetsLow[index] = offset;
	}
}

void CFrontend::sendMotorCommand(uint8_t cmdtype, uint8_t address, uint8_t command, uint8_t num_parameters, uint8_t parameter1, uint8_t parameter2)
{
	struct dvb_diseqc_master_cmd cmd;
	
	printf("[frontend] sendMotorCommand: cmdtype   = %x, address = %x, cmd   = %x\n", cmdtype, address, command);
	printf("[frontend] sendMotorCommand: num_parms = %d, parm1   = %x, parm2 = %x\n", num_parameters, parameter1, parameter2);
	
	cmd.msg[0] = cmdtype; //command type
	cmd.msg[1] = address; //address
	cmd.msg[2] = command; //command
	cmd.msg[3] = parameter1;
	cmd.msg[4] = parameter2;
	cmd.msg_len = 3 + num_parameters;
	
	sendDiseqcCommand(&cmd, 15);
	printf("[frontend] motor positioning command sent.\n");
}

void CFrontend::positionMotor(uint8_t motorPosition)
{
	struct dvb_diseqc_master_cmd cmd;
	
	if (motorPosition != 0)
	{
		cmd.msg[0] = 0xE0; //command type
		cmd.msg[1] = 0x31; //address
		cmd.msg[2] = 0x6B; //command: goto stored motor position
		cmd.msg[3] = motorPosition;
		cmd.msg_len = 4;
		sendDiseqcCommand(&cmd, 15);
		printf("[frontend] motor positioning command sent.\n");
	}
}

int CFrontend::setParameters(struct dvb_frontend_parameters *feparams, const uint8_t polarization, const uint8_t diseqc)
{
	int ret, freq_offset = 0;

	if (info.type == FE_QPSK) {
		bool high_band;

		if (feparams->frequency < 11700000) {
			high_band = false;
			freq_offset = lnbOffsetsLow[diseqc];
		}
		else {
			high_band = true;
			freq_offset = lnbOffsetsHigh[diseqc];
		}

		feparams->frequency -= freq_offset;
		setSec(diseqc, polarization, high_band, feparams->frequency);
	}



	/*
	 * frontends which can not handle auto inversion but
	 * are set to auto inversion will try without first
	 */

	bool sw_auto_inversion;

	if ((feparams->inversion == INVERSION_AUTO) && (!(info.caps & FE_CAN_INVERSION_AUTO)))
		sw_auto_inversion = true;
	else
		sw_auto_inversion = false;



	/*
	 * frontends which can not handle auto modulation detection
	 * are set to QAM64 which is common in Germany.
	 */

	if ((feparams->u.qam.modulation == QAM_AUTO) && (!(info.caps & FE_CAN_QAM_AUTO)))
		feparams->u.qam.modulation = QAM_64;



	struct dvb_frontend_event event;

	do {
		tuned = false;

		setFrontend(feparams);
		event = getEvent();

		/*
		 * software auto inversion for stupid frontends
		 */

		if ((!tuned) && (sw_auto_inversion) && (feparams->inversion == INVERSION_OFF)) {
			feparams->inversion = INVERSION_ON;
			continue;
		}

	} while (0);

	if (tuned) {
		ret = diff(event.parameters.frequency, feparams->frequency);
#if 1
		/*
		 * if everything went ok, then it is a good idea to copy the real
		 * frontend parameters, so we can update the service list, if it differs.
		 *
		 * TODO: set a flag to indicate a change in the service list
		 */

		if (memcmp(feparams, &event.parameters, sizeof(struct dvb_frontend_parameters)))
			memcpy(feparams, &event.parameters, sizeof(struct dvb_frontend_parameters));
#endif
	}

	else {
		ret = -1;
	}



	/*
	 * add the frequency offset to the frontend parameters again
	 * because they are used for the channel list and were given
	 * to this method as a pointer
	 */

	if (info.type == FE_QPSK)
		feparams->frequency += freq_offset;

	return ret;
}

void CFrontend::setSec(const uint8_t sat_no, const uint8_t pol, const bool high_band, const uint32_t frequency)
{
	uint8_t repeats = diseqcRepeats;

	fe_sec_voltage_t v = (pol & 1) ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18;
	fe_sec_tone_mode_t t = high_band ? SEC_TONE_ON : SEC_TONE_OFF;
	fe_sec_mini_cmd_t b = (sat_no & 1) ? SEC_MINI_B : SEC_MINI_A;

	/*
	 * [0] from master, no reply, 1st transmission
	 * [1] any lnb switcher or smatv
	 * [2] write to port group 0 (committed switches)
	 * [3] high nibble: reset bits, low nibble set bits
	 *     bits are: option, position, polarizaion, band
	 */

	struct dvb_diseqc_master_cmd cmd = {
		{ 0xe0, 0x10, 0x38, 0x00, 0x00, 0x00 }, 4
	};

	cmd.msg[3] = 0xf0 | (((sat_no * 4) & 0x0f) | (high_band ? 1 : 0) | ((pol & 1) ? 0 : 2));

	/*
	 * set all SEC / DiSEqC parameters
	 */

	secSetTone(SEC_TONE_OFF, 15);
	secSetVoltage(v, 15);

	if (diseqcType >= SMATV_REMOTE_TUNING) {

		if (diseqcType >= DISEQC_2_0)
			cmd.msg[0] |= 0x02;	/* reply required */

		sendDiseqcCommand(&cmd, 15);

		if (diseqcType >= DISEQC_2_0)
			repeats += getDiseqcReply(50);

	}

	if ((diseqcType >= DISEQC_1_1) && (repeats)) {
		for (uint16_t i = 0; i < repeats; i++) {
			uint8_t again = 0;

			usleep(1000 * 100);	/* wait at least 100ms before retransmission */

			cmd.msg[2] |= 0x01;	/* uncommitted switches */
			sendDiseqcCommand(&cmd, 15);

			if (diseqcType >= DISEQC_2_0)
				again += getDiseqcReply(50);

			cmd.msg[0] |= 0x01;	/* repeated transmission */
			cmd.msg[2] &= 0xFE;	/* committed switches */
			sendDiseqcCommand(&cmd, 15);

			if (diseqcType >= DISEQC_2_0)
				again += getDiseqcReply(50);

			if (again == 2)
				repeats++;
		}
	}

	if (diseqcType == SMATV_REMOTE_TUNING)
		sendDiseqcSmatvRemoteTuningCommand(frequency);

	if (diseqcType == MINI_DISEQC)
		sendToneBurst(b, 15);

	secSetTone(t, 15);

	currentDiseqc = sat_no;
}

void CFrontend::sendDiseqcPowerOn(void)
{
	sendDiseqcZeroByteCommand(0xe0, 0x10, 0x03);
}

void CFrontend::sendDiseqcReset(void)
{
	/* Reset && Clear Reset */
	sendDiseqcZeroByteCommand(0xe0, 0x10, 0x00);
	sendDiseqcZeroByteCommand(0xe0, 0x10, 0x01);
}

void CFrontend::sendDiseqcStandby(void)
{
	sendDiseqcZeroByteCommand(0xe0, 0x10, 0x02);
}

void CFrontend::sendDiseqcZeroByteCommand(uint8_t framing_byte, uint8_t address, uint8_t command)
{
	struct dvb_diseqc_master_cmd diseqc_cmd = {
		{ framing_byte, address, command, 0x00, 0x00, 0x00 }, 3
	};

	sendDiseqcCommand(&diseqc_cmd, 15);
}

void CFrontend::sendDiseqcSmatvRemoteTuningCommand(const uint32_t frequency)
{
 	/*
	 * [0] from master, no reply, 1st transmission
	 * [1] intelligent slave interface for multi-master bus
	 * [2] write channel frequency
	 * [3] frequency
	 * [4] frequency
	 * [5] frequency
	 */

	struct dvb_diseqc_master_cmd cmd = {
		{ 0xe0, 0x71, 0x58, 0x00, 0x00, 0x00 }, 6
	};

	cmd.msg[3] = (((frequency / 10000000) << 4) & 0xF0) | ((frequency / 1000000) & 0x0F);
	cmd.msg[4] = (((frequency / 100000) << 4) & 0xF0) | ((frequency / 10000) & 0x0F);
	cmd.msg[5] = (((frequency / 1000) << 4) & 0xF0) | ((frequency / 100) & 0x0F);

	sendDiseqcCommand(&cmd, 15);
}

