#ifndef SND_TDA998X_H
#define SND_TDA998X_H

/* port index for audio stream stop */
#define PORT_NONE (-1)

struct tda998x_audio {
	u8 ports[2];			/* AP value */
	u8 port_types[2];		/* AFMT_xxx */
	u8 *eld;
	int (*set_audio_input)(struct device *dev,
			int port_index,
			unsigned sample_rate);
};

int tda9998x_codec_register(struct device *dev);
void tda9998x_codec_unregister(struct device *dev);
#endif
