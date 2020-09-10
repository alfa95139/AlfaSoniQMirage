
uint8_t doc5503_irq();
void doc_init();
void doc_run();
uint8_t doc_rreg(uint8_t reg);
void doc_wreg(uint8_t reg, uint8_t val);


enum {
	MODE_FREE = 0,
	MODE_ONESHOT = 1,
	MODE_SYNCAM = 2,
	MODE_SWAP = 3
	};

struct DOC5503Osc
	{
		uint16_t freq;
		uint16_t wtsize;
		uint8_t  control;
		uint8_t  vol;
		uint8_t  data;
		uint32_t wavetblpointer; // DOC5503 implementation uses 8bits, but we need 17 bits!!!
		uint8_t  wavetblsize;
		uint8_t  resolution;

		uint32_t accumulator;
		uint8_t  irqpend;
	};

	DOC5503Osc oscillators[32];

	uint8_t  oscsenabled;      // # of oscillators enabled
	uint8_t     regE0, regE1, regE2;            // contents of register 0xe0

	uint8_t m_channel_strobe;

	int output_channels;
	uint32_t output_rate;

   uint8_t doc_irq;
