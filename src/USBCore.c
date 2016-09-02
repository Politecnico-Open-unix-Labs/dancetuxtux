/* Copyright (c) 2010, Peter Barrett
** Sleep/Wakeup support added by Michael Dreher
**
** Permission to use, copy, modify, and/or distribute this software for
** any purpose with or without fee is hereby granted, provided that the
** above copyright notice and this permission notice appear in all copies.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
** SOFTWARE.
*/

#include "USB.h"

/** Pulse generation counters to keep track of the number of milliseconds remaining for each pulse type */
#define TX_RX_LED_PULSE_MS 100
volatile uint8_t TxLEDPulse; /**< Milliseconds remaining for data Tx LED pulse */
volatile uint8_t RxLEDPulse; /**< Milliseconds remaining for data Rx LED pulse */

//==================================================================
//==================================================================

extern const uint16_t STRING_LANGUAGE[] PROGMEM;
extern const uint8_t STRING_PRODUCT[] PROGMEM;
extern const uint8_t STRING_MANUFACTURER[] PROGMEM;
extern const DeviceDescriptor USB_DeviceDescriptor PROGMEM;
extern const DeviceDescriptor USB_DeviceDescriptorB PROGMEM;

const uint16_t STRING_LANGUAGE[2] = {
	(3<<8) | (2+2),
	0x0409	// English
};

#define USB_PRODUCT "DDR Controller"
#define USB_MANUFACTURER "AS"
const uint8_t STRING_PRODUCT[] PROGMEM = USB_PRODUCT;
const uint8_t STRING_MANUFACTURER[] PROGMEM = USB_MANUFACTURER;

#define DEVICE_CLASS 0x02

//	DEVICE DESCRIPTOR
const DeviceDescriptor USB_DeviceDescriptor =
	D_DEVICE(0x00,0x00,0x00,64,USB_VID,USB_PID,0x100,IMANUFACTURER,IPRODUCT,ISERIAL,1);

const DeviceDescriptor USB_DeviceDescriptorB =
	D_DEVICE(0xEF,0x02,0x01,64,USB_VID,USB_PID,0x100,IMANUFACTURER,IPRODUCT,ISERIAL,1);

static uint8_t _cdcComposite = 0;
static const uint8_t _hidReportDescriptor[] PROGMEM = {
    0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0x85, 0x02, 0x05, 0x07, 0x19, 0xe0, 0x29,
    0xe7, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02, 0x95, 0x01,
    0x75, 0x08, 0x81, 0x03, 0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x25, 0x65, 0x05,
    0x07, 0x19, 0x00, 0x29, 0x65, 0x81, 0x00, 0xc0,
};

//==================================================================
//==================================================================

volatile uint8_t _usbConfiguration = 0;
volatile uint8_t _usbCurrentStatus = 0; // meaning of bits see usb_20.pdf, Figure 9-4. Information Returned by a GetStatus() Request to a Device
volatile uint8_t _usbSuspendState = 0; // copy of UDINT to check SUSPI and WAKEUPI bits

static inline void WaitIN(void)  {   while (!(UEINTX & (1<<TXINI)));   }
static inline void ClearIN(void) {   UEINTX = ~(1<<TXINI);   }
static inline void WaitOUT(void) {   while (!(UEINTX & (1<<RXOUTI)));   }
static inline uint8_t WaitForINOrOUT(void) { while (!(UEINTX & ((1<<TXINI)|(1<<RXOUTI)))); return (UEINTX & (1<<RXOUTI)) == 0; }
static inline void ClearOUT(void) { UEINTX = ~(1<<RXOUTI); }
static inline void Recv(volatile uint8_t* data, uint8_t count) { while (count--) *data++ = UEDATX; RXLED1; RxLEDPulse = TX_RX_LED_PULSE_MS; }
static inline uint8_t Recv8(void) { RXLED1; RxLEDPulse = TX_RX_LED_PULSE_MS; return UEDATX; }
static inline void Send8(uint8_t d) { UEDATX = d; }
static inline void SetEP(uint8_t ep) { UENUM = ep; }
static inline uint8_t FifoByteCount(void) { return UEBCLX; }
static inline uint8_t ReceivedSetupInt(void) { return UEINTX & (1<<RXSTPI); }
static inline void ClearSetupInt(void) { UEINTX = ~((1<<RXSTPI) | (1<<RXOUTI) | (1<<TXINI)); }
static inline void Stall(void) { UECONX = (1<<STALLRQ) | (1<<EPEN); }
static inline uint8_t ReadWriteAllowed(void) { return UEINTX & (1<<RWAL); }
static inline uint8_t Stalled(void) { return UEINTX & (1<<STALLEDI); }
static inline uint8_t FifoFree(void) { return UEINTX & (1<<FIFOCON); }
static inline void ReleaseRX(void) {
	UEINTX = 0x6B;	// FIFOCON=0 NAKINI=1 RWAL=1 NAKOUTI=0 RXSTPI=1 RXOUTI=0 STALLEDI=1 TXINI=1
}
static inline void ReleaseTX(void) {
	UEINTX = 0x3A;	// FIFOCON=0 NAKINI=0 RWAL=1 NAKOUTI=1 RXSTPI=1 RXOUTI=0 STALLEDI=1 TXINI=0
}
static inline uint8_t FrameNumber(void) { return UDFNUML; }

//==================================================================
//==================================================================

uint8_t USBGetConfiguration(void) { return _usbConfiguration; }

//	Number of bytes, assumes a rx endpoint
uint8_t USB_Available(uint8_t ep) {
	uint8_t ret, _sreg = SREG;
	cli();
	SetEP(ep & 7);
	ret = FifoByteCount();
	SREG = _sreg;
	return ret;
}

//	Non Blocking receive
//	Return number of bytes read
int USB_Recv(uint8_t ep, void* d, int len) {
	uint8_t _sreg;
	if (!_usbConfiguration || len < 0) return -1;
	_sreg = SREG; cli();
	SetEP(ep & 7);
	uint8_t n = FifoByteCount();
	len = (n < len)?n:len; // min(n, len)
	n = len;
	uint8_t* dst = (uint8_t*)d;
	while (n--)
		*dst++ = Recv8();
	if (len && !FifoByteCount())	// release empty buffer
		ReleaseRX();
	SREG = _sreg;
	return len;
}

//	Recv 1 byte if ready
int USB_Recv1(uint8_t ep) {
	uint8_t c;
	if (USB_Recv(ep,&c,1) != 1)
		return -1;
	return c;
}

//	Space in send EP
uint8_t USB_SendSpace(uint8_t ep) {
	uint8_t _sreg, ret;
	_sreg = SREG; cli();
	SetEP(ep & 7);
	if (!ReadWriteAllowed()) ret = 0;
	else ret = USB_EP_SIZE - 1 - FifoByteCount();
	// subtract 1 from the EP size to never send a full packet,
	// this avoids dealing with ZLP's in USB_Send
	SREG = _sreg;
	return ret;
}

//	Blocking Send of data to an endpoint
int USB_Send(uint8_t ep, const void* d, int len) {
	if (!_usbConfiguration)
		return -1;

	if (_usbSuspendState & (1<<SUSPI)) {
		//send a remote wakeup
		UDCON |= (1 << RMWKUP);
	}

	int r = len;
	const uint8_t* data = (const uint8_t*)d;
	uint8_t timeout = 250;		// 250ms timeout on send? TODO
	while (len) {
		uint8_t n = USB_SendSpace(ep);
		if (n == 0) {
			if (!(--timeout)) return -1;
			_delay_ms(1);
			continue;
		}

		if (n > len) { n = len; }

		uint8_t _sreg = SREG; cli();
		SetEP(ep & 7);
		// Frame may have been released by the SOF interrupt handler
		if (!ReadWriteAllowed()) { SREG = _sreg; continue; }
		len -= n;
		if (ep & TRANSFER_ZERO)
			while (n--) Send8(0);
		else if (ep & TRANSFER_PGM)
			while (n--) Send8(pgm_read_byte(data++));
		else
			while (n--) Send8(*data++);
		if (!ReadWriteAllowed() || ((len == 0) && (ep & TRANSFER_RELEASE)))	// Release full buffer
			ReleaseTX();
		SREG = _sreg;
	}
	TXLED1;					// light the TX LED
	TxLEDPulse = TX_RX_LED_PULSE_MS;
	return r;
}

uint8_t _initEndpoints[USB_ENDPOINTS] = { 0, EP_TYPE_INTERRUPT_IN, EP_TYPE_BULK_OUT, EP_TYPE_BULK_IN, };

#define EP_SINGLE_64 0x32	// EP0
#define EP_DOUBLE_64 0x36	// Other endpoints
#define EP_SINGLE_16 0x12

static void InitEP(uint8_t index, uint8_t type, uint8_t size) {
	UENUM = index;
	UECONX = (1<<EPEN);
	UECFG0X = type;
	UECFG1X = size;
}

static void InitEndpoints(void) {
	for (uint8_t i = 1; i < sizeof(_initEndpoints) && _initEndpoints[i] != 0; i++) {
		UENUM = i;
		UECONX = (1<<EPEN);
		UECFG0X = _initEndpoints[i];
		UECFG1X = EP_DOUBLE_64;
	}
	UERST = 0x7E;	// And reset them
	UERST = 0;
}

//	Handle CLASS_INTERFACE requests
static bool ClassInterfaceRequest(USBSetup* setup) {
	uint8_t i = setup->wIndex;
	uint8_t request = setup->bRequest;
	uint8_t requestType = setup->bmRequestType;

	if (CDC_ACM_INTERFACE == i) return true;
	 /* return PluggableUSB().setup(setup); */

	if (2 /* pluggedInterface */ != setup->wIndex) return false;

	if (requestType == REQUEST_DEVICETOHOST_CLASS_INTERFACE) {
		if (request == HID_GET_REPORT) return true;
		if (request == HID_GET_PROTOCOL) return true;
		if (request == HID_GET_IDLE) {}
	}

	if (requestType == REQUEST_HOSTTODEVICE_CLASS_INTERFACE) {
		if (request == HID_SET_PROTOCOL) {
			// protocol = setup->wValueL;
			return true;
		} if (request == HID_SET_IDLE) {
			// idle = setup->wValueL;
			return true;
		} if (request == HID_SET_REPORT) {}
	}

	 return false;

}

static int _cmark;
static int _cend;
void InitControl(int end) {
	SetEP(0);
	_cmark = 0;
	_cend = end;
}

static bool SendControl(uint8_t d) {
	if (_cmark < _cend) {
		if (!WaitForINOrOUT())
			return false;
		Send8(d);
		if (!((_cmark + 1) & 0x3F))
			ClearIN();	// Fifo is full, release this packet
	}
	_cmark++;
	return true;
}

//	Clipped by _cmark/_cend
int USB_SendControl(uint8_t flags, const void* d, int len) {
	int sent = len;
	const uint8_t* data = (const uint8_t*)d;
	bool pgm = flags & TRANSFER_PGM;
	while (len--) {
		uint8_t c = pgm ? pgm_read_byte(data++) : *data++;
		if (!SendControl(c))
			return -1;
	}
	return sent;
}

// Send a USB descriptor string. The string is stored in PROGMEM as a
// plain ASCII string but is sent out as UTF-16 with the correct 2-byte
// prefix
static bool USB_SendStringDescriptor(const uint8_t*string_P, uint8_t string_len, uint8_t flags) {
        SendControl(2 + string_len * 2);
        SendControl(3);
        bool pgm = flags & TRANSFER_PGM;
        for(uint8_t i = 0; i < string_len; i++) {
                bool r = SendControl(pgm ? pgm_read_byte(&string_P[i]) : string_P[i]);
                r &= SendControl(0); // high byte
                if(!r) {
                        return false;
                }
        }
        return true;
}

//	Does not timeout or cross fifo boundaries
int USB_RecvControl(void* d, int len) {
	int length = len;
	while(length) {
		// Dont receive more than the USB Control EP has to offer
		// Use fixed 64 because control EP always have 64 bytes even on 16u2.
		int recvLength = length;
		if(recvLength > 64) {
			recvLength = 64;
		}

		// Write data to fit to the end (not the beginning) of the array
		WaitOUT();
		Recv((uint8_t*)d + len - length, recvLength);
		ClearOUT();
		length -= recvLength;
	}
	return len;
}

static uint8_t SendInterfaces(void) {
	/* uint8_t interfaces = 0; */
	/* PluggableUSB().getInterface(&interfaces); */
	HIDDescriptor hidInterface = {
		D_INTERFACE(2 /* pluggedInterface */, 1, USB_DEVICE_CLASS_HUMAN_INTERFACE, HID_SUBCLASS_NONE, HID_PROTOCOL_NONE),
		D_HIDREPORT(47/* descriptorSize */), // <<<<< LENGHT OF THE ARRAY
		D_ENDPOINT(USB_ENDPOINT_IN(4 /* pluggedEndpoint */), USB_ENDPOINT_TYPE_INTERRUPT, USB_EP_SIZE, 0x01)
	};
	int res = USB_SendControl(0, &hidInterface, sizeof(hidInterface));
	if (res < 0)
		return -1;
	return 1; // only one interface
}

static bool SendConfiguration(int maxlen) {
	//	Count and measure interfaces
	InitControl(0);
	uint8_t interfaces = SendInterfaces();
	ConfigDescriptor config = D_CONFIG(_cmark + sizeof(ConfigDescriptor),interfaces);

	//	Now send them
	InitControl(maxlen);
	USB_SendControl(0,&config,sizeof(ConfigDescriptor));
	SendInterfaces();
	return true;
}

static bool SendDescriptor(USBSetup* setup) {
	int ret;
	uint8_t t = setup->wValueH;
	if (USB_CONFIGURATION_DESCRIPTOR_TYPE == t)
		return SendConfiguration(setup->wLength);

	InitControl(setup->wLength);
	/* ret = PluggableUSB().getDescriptor(setup); */

	// Check if this is a HID Class Descriptor request
	if (setup->bmRequestType != REQUEST_DEVICETOHOST_STANDARD_INTERFACE) {
		ret = 0;
	} else if (setup->wValueH != HID_REPORT_DESCRIPTOR_TYPE) {
		ret = 0;
	} else if (setup->wIndex != 2 /* pluggedInterface */) {
		// In a HID Class Descriptor wIndex cointains the interface number
		ret = 0;
	} else {
		int res = USB_SendControl(TRANSFER_PGM, _hidReportDescriptor, sizeof(_hidReportDescriptor));
		if (res == -1)
			return -1;
		ret = res;
		/* NOTE: protocol = HID_REPORT_PROTOCOL; */
	}

	if (ret != 0) {
		return (ret > 0 ? true : false);
	}

	const uint8_t* desc_addr = 0;
	if (USB_DEVICE_DESCRIPTOR_TYPE == t) {
		if (setup->wLength == 8)
			_cdcComposite = 1;
		desc_addr = _cdcComposite ?  (const uint8_t*)&USB_DeviceDescriptorB : (const uint8_t*)&USB_DeviceDescriptor;
	} else if (USB_STRING_DESCRIPTOR_TYPE == t) {
		if (setup->wValueL == 0) {
			desc_addr = (const uint8_t*)&STRING_LANGUAGE;
		}
		else if (setup->wValueL == IPRODUCT) {
			return USB_SendStringDescriptor(STRING_PRODUCT, strlen(USB_PRODUCT), TRANSFER_PGM);
		}
		else if (setup->wValueL == IMANUFACTURER) {
			return USB_SendStringDescriptor(STRING_MANUFACTURER, strlen(USB_MANUFACTURER), TRANSFER_PGM);
		}
		else if (setup->wValueL == ISERIAL) {
			char name[ISERIAL_MAX_LEN];
			/* PluggableUSB().getShortName(name); */
			name[0] = 'H'; name[1] = 'I'; name[2] = 'D';
			name[3] = 'A' + (47 /* descriptorSize */ & 0x0F);
			name[4] = 'A' + ((47 /* descriptorSize */ >> 4) & 0x0F);
			name[5] = '\0';
			return USB_SendStringDescriptor((uint8_t*)name, strlen(name), 0);
		}
		else
			return false;
	}

	if (desc_addr == 0)
		return false;
	uint8_t desc_length = pgm_read_byte(desc_addr);

	USB_SendControl(TRANSFER_PGM,desc_addr,desc_length);
	return true;
}

//	Endpoint 0 interrupt
static volatile uint8_t usb_init_done = 0; // Becames 1 when initialization has done
ISR(USB_COM_vect) {
    SetEP(0);
	if (!ReceivedSetupInt())
		return;

	USBSetup setup;
	Recv((uint8_t*)&setup,8);
	ClearSetupInt();

	uint8_t requestType = setup.bmRequestType;
	if (requestType & REQUEST_DEVICETOHOST)
		WaitIN();
	else
		ClearIN();

    bool ok = true;
	if (REQUEST_STANDARD == (requestType & REQUEST_TYPE)) {
		//	Standard Requests
		uint8_t r = setup.bRequest;
		uint16_t wValue = setup.wValueL | (setup.wValueH << 8);
		if (GET_STATUS == r) {
			if (requestType == (REQUEST_DEVICETOHOST | REQUEST_STANDARD | REQUEST_DEVICE)){
				Send8(_usbCurrentStatus);
				Send8(0);
			} else {
				Send8(0);
				Send8(0);
			}
		} else if (CLEAR_FEATURE == r) {
			if((requestType == (REQUEST_HOSTTODEVICE | REQUEST_STANDARD | REQUEST_DEVICE))
				&& (wValue == DEVICE_REMOTE_WAKEUP)) {
				_usbCurrentStatus &= ~FEATURE_REMOTE_WAKEUP_ENABLED;
			}
		} else if (SET_FEATURE == r) {
			if((requestType == (REQUEST_HOSTTODEVICE | REQUEST_STANDARD | REQUEST_DEVICE))
				&& (wValue == DEVICE_REMOTE_WAKEUP)) {
				_usbCurrentStatus |= FEATURE_REMOTE_WAKEUP_ENABLED;
			}
		} else if (SET_ADDRESS == r) {
			WaitIN();
			UDADDR = setup.wValueL | (1<<ADDEN);
		} else if (GET_DESCRIPTOR == r) {
			ok = SendDescriptor(&setup);
		} else if (SET_DESCRIPTOR == r) {
			ok = false;
		} else if (GET_CONFIGURATION == r) {
			Send8(1);
		}
		else if (SET_CONFIGURATION == r) {
			if (REQUEST_DEVICE == (requestType & REQUEST_RECIPIENT)) {
				InitEndpoints();
				_usbConfiguration = setup.wValueL;
			} else {
				ok = false;
			}
		}
		else if (GET_INTERFACE == r) {}
		else if (SET_INTERFACE == r) {}
	}
	else {
		InitControl(setup.wLength);		//	Max length of transfer
		ok = ClassInterfaceRequest(&setup);
	}

	if (ok) {
		ClearIN();
		usb_init_done = 1; // TODO: maybe initialization got here many times
	} else {
		Stall();
	}
}

void USB_Flush(uint8_t ep) {
	SetEP(ep);
	if (FifoByteCount())
		ReleaseTX();
}

static inline void USB_ClockDisable(void) {
	USBCON = (USBCON & ~(1<<OTGPADE)) | (1<<FRZCLK); // freeze clock and disable VBUS Pad
	PLLCSR &= ~(1<<PLLE);  // stop PLL
}

static inline void USB_ClockEnable(void) {
	UHWCON |= (1<<UVREGE);			// power internal reg
	USBCON = (1<<USBE) | (1<<FRZCLK);	// clock frozen, usb enabled
	PLLCSR |= (1<<PINDIV);                   // Need 16 MHz xtal
	PLLCSR |= (1<<PLLE);
	while (!(PLLCSR & (1<<PLOCK)));		// wait for lock pll
	_delay_ms(1);
	USBCON = (USBCON & ~(1<<FRZCLK)) | (1<<OTGPADE);	// start USB clock, enable VBUS Pad
	UDCON &= ~((1<<RSTCPU) | (1<<LSM) | (1<<RMWKUP) | (1<<DETACH));	// enable attach resistor, set full speed mode
}

//	General interrupt
ISR(USB_GEN_vect) {
	uint8_t udint = UDINT;
	UDINT &= ~((1<<EORSTI) | (1<<SOFI)); // clear the IRQ flags for the IRQs which are handled here, except WAKEUPI and SUSPI (see below)

	//	End of Reset
	if (udint & (1<<EORSTI)) {
		InitEP(0,EP_TYPE_CONTROL,EP_SINGLE_64);	// init ep0
		_usbConfiguration = 0;			// not configured yet
		UEIENX = 1 << RXSTPE;			// Enable interrupts for ep0
	}

	//	Start of Frame - happens every millisecond so we use it for TX and RX LED one-shot timing, too
	if (udint & (1<<SOFI)) {
		USB_Flush(CDC_TX);				// Send a tx frame if found
		// check whether the one-shot period has elapsed.  if so, turn off the LED
		if (TxLEDPulse && !(--TxLEDPulse))
			TXLED0;
		if (RxLEDPulse && !(--RxLEDPulse))
			RXLED0;
	}

	// the WAKEUPI interrupt is triggered as soon as there are non-idle patterns on the data
	// lines. Thus, the WAKEUPI interrupt can occur even if the controller is not in the "suspend" mode.
	// Therefore the we enable it only when USB is suspended
	if (udint & (1<<WAKEUPI)) {
		UDIEN = (UDIEN & ~(1<<WAKEUPE)) | (1<<SUSPE); // Disable interrupts for WAKEUP and enable interrupts for SUSPEND
		UDINT &= ~(1<<WAKEUPI);
		_usbSuspendState = (_usbSuspendState & ~(1<<SUSPI)) | (1<<WAKEUPI);
	} else if (udint & (1<<SUSPI)) { // only one of the WAKEUPI / SUSPI bits can be active at time
		UDIEN = (UDIEN & ~(1<<SUSPE)) | (1<<WAKEUPE); // Disable interrupts for SUSPEND and enable interrupts for WAKEUP
		UDINT &= ~((1<<WAKEUPI) | (1<<SUSPI)); // clear any already pending WAKEUP IRQs and the SUSPI request
		_usbSuspendState = (_usbSuspendState & ~(1<<WAKEUPI)) | (1<<SUSPI);
	}
}

//=======================================================================
//=======================================================================
uint8_t report[8];

void __USB_init(void) {
    sei(); // Enables interrupts
	_usbConfiguration = 0;
	_usbCurrentStatus = 0;
	_usbSuspendState = 0;
	USB_ClockEnable();

	UDINT &= ~((1<<WAKEUPI) | (1<<SUSPI)); // clear already pending WAKEUP / SUSPEND requests
	UDIEN = (1<<EORSTE) | (1<<SOFE) | (1<<SUSPE);	// Enable interrupts for EOR (End of Reset), SOF (start of frame) and SUSPEND

	TX_RX_LED_INIT;

	_initEndpoints[4] = EP_TYPE_INTERRUPT_IN;
	memset(report, 0, sizeof(report)); // sets all of the report to zero
	while (usb_init_done == 0) { _delay_ms(1); } // Waits one millisecond before each test
	_delay_ms(250); // Delays further time
}

int __SendReport(uint8_t id, const void* data, int len) {
    int ret = USB_Send(4, &id, 1);
    if (ret < 0) return ret;
    int ret2 = USB_Send(4 | TRANSFER_RELEASE, data, len);
    if (ret2 < 0) return ret2;
    return ret + ret2;
}

void __USB_send_keypress(uint8_t key_code) {
    uint8_t i;
    const uint8_t len = sizeof(report)/sizeof(*report);
    for (i = 2; i < len; i++) {
        if (report[i] == 0) { // inserts key code at first non-null place
            report[i] = key_code;
            break; // Goes to sendreport
        } else if (report[i] == key_code) { // avoid inserting twice the same key code
            i = len; // Goes to the end of the array
            break; // nothing has changed
        }
    }
    if (i != len) // If not reached the end
        __SendReport(2, report, sizeof(report));
}

void __USB_send_keyrelease(uint8_t key_code) {
    uint8_t i;
    const uint8_t len = sizeof(report)/sizeof(*report);
    for (i = 2; i < len; i++) {
        if (report[i] == 0) {
            i = len; // Goes to the end of the array
            break; // No more codes, nothing has changed
        } else if (report[i] == key_code) {
            memmove(report + i, report + i + 1, (len - i - 1)*sizeof(*report));
            report[len-1] = 0;
            break; // Goes to sendreport
        }
    }
    if (i != len) // If not reached the end
        __SendReport(2, report, sizeof(report));
}

// DEBUG PURPOSE
#ifndef NDEBUG

static int __ascii_to_usb(const uint8_t ch) {
    if (ch >= 'A' && ch <= 'Z') return ch - 'A' + 0x04; // Uppercase
    if (ch >= 'a' && ch <= 'z') return ch - 'a' + 0x04; // Lowercase
    if (ch >= '0' && ch <= '9') { if (ch == '0') return 0x27; else return ch - '1' + 0x1e;} // Digit
    if (ch == ' ') return 0x2c;
	// Etc...
    return 0x00;
}

void __USB_send_string(const uint8_t * msg) {
    uint8_t i = 0;
    while (msg[i]) {
        __USB_send_keypress(__ascii_to_usb(msg[i]));
        __USB_send_keyrelease(__ascii_to_usb(msg[i]));
        i++;
    }
}

#endif // NDEBUG defined
