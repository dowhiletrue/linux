/*
 * ALSA SoC TDA998x CODEC
 *
 * Copyright (C) 2015 Jean-Francois Moine
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <sound/soc.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <sound/pcm_drm_eld.h>
#include <drm/i2c/tda998x.h>
#include <sound/tda998x.h>

static int tda998x_codec_startup(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct tda998x_audio *tda998x_audio = dev_get_drvdata(dai->dev);
	u8 *eld;

	eld = tda998x_audio->eld;
	if (!eld)
		return -ENODEV;
	return snd_pcm_hw_constraint_eld(runtime, eld);
}

/* ask the HDMI transmitter to activate the audio input port */
static int tda998x_codec_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params,
				   struct snd_soc_dai *dai)
{
	struct tda998x_audio *tda998x_audio = dev_get_drvdata(dai->dev);

	return tda998x_audio->set_audio_input(dai->dev, dai->id,
					params_rate(params));
}

static void tda998x_codec_shutdown(struct snd_pcm_substream *substream,
				   struct snd_soc_dai *dai)
{
	struct tda998x_audio *tda998x_audio = dev_get_drvdata(dai->dev);

	tda998x_audio->set_audio_input(dai->dev, PORT_NONE, 0);
}

static const struct snd_soc_dai_ops tda998x_codec_ops = {
	.startup = tda998x_codec_startup,
	.hw_params = tda998x_codec_hw_params,
	.shutdown = tda998x_codec_shutdown,
};

#define TDA998X_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE | \
			SNDRV_PCM_FMTBIT_S20_3LE | \
			SNDRV_PCM_FMTBIT_S24_LE | \
			SNDRV_PCM_FMTBIT_S32_LE)

static const struct snd_soc_dai_driver tda998x_dai_i2s = {
	.name = "i2s-hifi",
	.playback = {
		.stream_name	= "HDMI I2S Playback",
		.channels_min	= 1,
		.channels_max	= 8,
		.rates		= SNDRV_PCM_RATE_CONTINUOUS,
		.rate_min	= 5512,
		.rate_max	= 192000,
		.formats	= TDA998X_FORMATS,
	},
	.ops = &tda998x_codec_ops,
};
static const struct snd_soc_dai_driver tda998x_dai_spdif = {
	.name = "spdif-hifi",
	.playback = {
		.stream_name	= "HDMI SPDIF Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_CONTINUOUS,
		.rate_min	= 22050,
		.rate_max	= 192000,
		.formats	= TDA998X_FORMATS,
	},
	.ops = &tda998x_codec_ops,
};

static const struct snd_soc_dapm_widget tda998x_widgets[] = {
	SND_SOC_DAPM_OUTPUT("hdmi-out"),
};
static const struct snd_soc_dapm_route tda998x_routes[] = {
	{ "hdmi-out", NULL, "HDMI I2S Playback" },
	{ "hdmi-out", NULL, "HDMI SPDIF Playback" },
};

static struct snd_soc_codec_driver tda998x_codec_drv = {
	.dapm_widgets = tda998x_widgets,
	.num_dapm_widgets = ARRAY_SIZE(tda998x_widgets),
	.dapm_routes = tda998x_routes,
	.num_dapm_routes = ARRAY_SIZE(tda998x_routes),
	.ignore_pmdown_time = true,
};

int tda9998x_codec_register(struct device *dev)
{
	struct snd_soc_dai_driver *dais, *p_dai;
	struct tda998x_audio *tda998x_audio = dev_get_drvdata(dev);
	int i, ndais;

	/* build the DAIs */
	for (ndais = 0; ndais < ARRAY_SIZE(tda998x_audio->ports); ndais++) {
		if (!tda998x_audio->ports[ndais])
			break;
	}
//test
//pr_info("tda998x codec %d dais\n", ndais);
	dais = devm_kzalloc(dev, sizeof(*dais) * ndais, GFP_KERNEL);
	if (!dais)
		return -ENOMEM;
	for (i = 0, p_dai = dais; i < ndais ; i++, p_dai++) {
		if (tda998x_audio->port_types[i] == AFMT_I2S)
			memcpy(p_dai, &tda998x_dai_i2s, sizeof(*p_dai));
		else
			memcpy(p_dai, &tda998x_dai_spdif, sizeof(*p_dai));
		p_dai->id = i;
//test
//pr_info("tda998x codec %d %s\n", p_dai->id, p_dai->name);
	}

	return snd_soc_register_codec(dev,
				&tda998x_codec_drv,
				dais, ndais);
}
EXPORT_SYMBOL_GPL(tda9998x_codec_register);

void tda9998x_codec_unregister(struct device *dev)
{
	snd_soc_unregister_codec(dev);
}
EXPORT_SYMBOL_GPL(tda9998x_codec_unregister);

MODULE_AUTHOR("Jean-Francois Moine <moinejf@free.fr>");
MODULE_DESCRIPTION("TDA998X CODEC");
MODULE_LICENSE("GPL");
