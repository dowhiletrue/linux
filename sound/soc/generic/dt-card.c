/*
 * ALSA SoC DT based sound card support
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

/* check if a node is an audio port */
static int asoc_dt_card_is_audio_port(struct device_node *of_port)
{
	const char *name;
	int ret;

	if (!of_port->name ||
	    of_node_cmp(of_port->name, "port") != 0)
		return 0;
	ret = of_property_read_string(of_port,
					"port-type",
					&name);
	if (!ret &&
	    (strcmp(name, "i2s") == 0 ||
	     strcmp(name, "spdif") == 0))
		return 1;
	return 0;
}

/*
 * Get the DAI number from the DT by counting the audio ports
 * of the remote device node (codec).
 */
static int asoc_dt_card_get_dai_number(struct device_node *of_codec,
				struct device_node *of_remote_endpoint)
{
	struct device_node *of_port, *of_endpoint;
	int ndai;

	ndai = 0;
	for_each_child_of_node(of_codec, of_port) {
		if (!asoc_dt_card_is_audio_port(of_port))
			continue;
		for_each_child_of_node(of_port, of_endpoint) {
			if (!of_endpoint->name ||
			    of_node_cmp(of_endpoint->name, "endpoint") != 0)
				continue;
			if (of_endpoint == of_remote_endpoint) {
				of_node_put(of_port);
				of_node_put(of_endpoint);
				return ndai;
			}
		}
		ndai++;
	}
	return 0;		/* should never be reached */
}

/*
 * Parse a graph of audio ports
 * @dev: Card device
 * @of_cpu: Device node of the audio controller
 * @card: Card definition
 *
 * Builds the DAI links of the card from the DT graph of audio ports
 * starting from the audio controller.
 * It does not handle the port groups.
 * The CODEC device nodes in the DAI links must be dereferenced by the caller.
 *
 * Returns the number of DAI links or (< 0) on error
 */
static int asoc_dt_card_of_parse_graph(struct device *dev,
				struct device_node *of_cpu,
				struct snd_soc_card *card)
{
	struct device_node *of_codec, *of_port, *of_endpoint,
				*of_remote_endpoint;
	struct snd_soc_dai_link *link;
	struct snd_soc_dai_link_component *component;
	struct of_phandle_args args, args2;
	int ret, ilink, icodec, nlinks, ncodecs;

//test
//pr_info("dt-card cpu %s\n", of_cpu->name ? of_cpu->name : "(null)");
	/* count the number of DAI links */
	nlinks = 0;
	for_each_child_of_node(of_cpu, of_port) {
//test
//pr_info("dt-card cpu child %s\n", of_port->name ? of_port->name : "(null)");
		if (asoc_dt_card_is_audio_port(of_port))
			nlinks++;
	}
//test
if (nlinks == 0) pr_info("dt-card no link!\n");

	/* allocate the DAI link array */
	link = devm_kzalloc(dev, sizeof(*link) * nlinks, GFP_KERNEL);
	if (!link)
		return -ENOMEM;
	card->dai_link = link;

	/* build the DAI links */
	ilink = 0;
	args.np = of_cpu;
	args.args_count = 1;
	for_each_child_of_node(of_cpu, of_port) {
		if (!asoc_dt_card_is_audio_port(of_port))
			continue;

		link->platform_of_node =
			link->cpu_of_node = of_cpu;
		args.args[0] = ilink;
		ret = snd_soc_get_dai_name(&args, &link->cpu_dai_name);
		if (ret) {
			dev_err(dev, "no CPU DAI name for link %d!\n",
				ilink);
			continue;
		}

		/* count the number of codecs of this DAI link */
		ncodecs = 0;
		for_each_child_of_node(of_port, of_endpoint) {
			if (of_parse_phandle(of_endpoint,
					"remote-endpoint", 0))
				ncodecs++;
		}
		if (ncodecs == 0)
			continue;
		component = devm_kzalloc(dev,
					 sizeof(*component) * ncodecs,
					 GFP_KERNEL);
		if (!component)
			return -ENOMEM;
		link->codecs = component;

		icodec = 0;
		args2.args_count = 1;
		for_each_child_of_node(of_port, of_endpoint) {
			of_remote_endpoint = of_parse_phandle(of_endpoint,
						"remote-endpoint", 0);
			if (!of_remote_endpoint)
				continue;
			component->of_node = of_codec =
					of_remote_endpoint->parent->parent;
			args2.np = of_codec;
			args2.args[0] = asoc_dt_card_get_dai_number(of_codec,
							of_remote_endpoint);
			ret = snd_soc_get_dai_name(&args2,
						   &component->dai_name);
			if (ret) {
				if (ret == -EPROBE_DEFER) {
					card->num_links = ilink + 1;
					link->num_codecs = icodec;
//test
//pr_info("dt-card probe defer\n");
					return ret;
				}
				dev_err(dev,
					"no CODEC DAI name for link %d\n",
					ilink);
				continue;
			}
//test
//pr_info("dt-card codec %d %s\n", args2.args[0], component->dai_name);
			of_node_get(of_codec);

			icodec++;
			if (icodec >= ncodecs)
				break;
			component++;
		}
		if (icodec == 0)
			continue;
		link->num_codecs = icodec;

		ilink++;
		if (ilink >= nlinks)
			break;
		link++;
	}
	card->num_links = ilink;

	return ilink;
}

static void asoc_dt_card_unref(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	struct snd_soc_dai_link *link;
	int nlinks, ncodecs;

	if (card) {
		for (nlinks = 0, link = card->dai_link;
		     nlinks < card->num_links;
		     nlinks++, link++) {
			for (ncodecs = 0;
			     ncodecs < link->num_codecs;
			     ncodecs++)
				of_node_put(card->dai_link->codecs[ncodecs].of_node);
		}
	}
}

/*
 * The platform data contains the pointer to the device node
 * which starts the description of the graph of the audio ports,
 * This device node is usually the audio controller.
 */
static int asoc_dt_card_probe(struct platform_device *pdev)
{
	struct device_node **p_np = pdev->dev.platform_data;
	struct device_node *of_cpu = *p_np;
	struct snd_soc_card *card;
	struct snd_soc_dai_link *link;
	char *name;
	int ret, i;

	card = devm_kzalloc(&pdev->dev, sizeof(*card), GFP_KERNEL);
	if (!card)
		return -ENOMEM;
	ret = asoc_dt_card_of_parse_graph(&pdev->dev, of_cpu, card);
	if (ret < 0)
		goto err;

	/* fill the remaining values of the card */
	card->owner = THIS_MODULE;
	card->dev = &pdev->dev;
	card->name = "DT-card";
	for (i = 0, link = card->dai_link;
	     i < card->num_links;
	     i++, link++) {
		name = devm_kzalloc(&pdev->dev,
				strlen(link->cpu_dai_name) +
					strlen(link->codecs[0].dai_name) +
					2,
				GFP_KERNEL);
		if (!name) {
			ret = -ENOMEM;
			goto err;
		}
		sprintf(name, "%s-%s", link->cpu_dai_name,
					link->codecs[0].dai_name);
		link->name = link->stream_name = name;
	}

	card->dai_link->dai_fmt =
		snd_soc_of_parse_daifmt(of_cpu, "dt-audio-card,",
					NULL, NULL) &
			~SND_SOC_DAIFMT_MASTER_MASK;

	ret = devm_snd_soc_register_card(&pdev->dev, card);
	if (ret >= 0)
		return ret;

err:
	asoc_dt_card_unref(pdev);
	return ret;
}

static int asoc_dt_card_remove(struct platform_device *pdev)
{
	asoc_dt_card_unref(pdev);
	snd_soc_unregister_card(platform_get_drvdata(pdev));
	return 0;
}

static struct platform_driver asoc_dt_card = {
	.driver = {
		.name = "asoc-dt-card",
	},
	.probe = asoc_dt_card_probe,
	.remove = asoc_dt_card_remove,
};

module_platform_driver(asoc_dt_card);

MODULE_ALIAS("platform:asoc-dt-card");
MODULE_DESCRIPTION("ASoC DT Sound Card");
MODULE_AUTHOR("Jean-Francois Moine <moinejf@free.fr>");
MODULE_LICENSE("GPL");
