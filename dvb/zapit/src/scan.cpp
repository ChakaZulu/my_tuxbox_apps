/*
 * $Id: scan.cpp,v 1.42 2002/04/19 01:14:24 obi Exp $
 */

#include "scan.h"

typedef std::map <uint32_t, scanchannel>::iterator sciterator;
typedef std::map <uint32_t, transpondermap>::iterator stiterator;
typedef std::multimap <std::string, bouquet_mulmap>::iterator sbiterator;

short scan_runs;
short curr_sat;

extern int found_transponders;
extern int found_channels;

/* zapit.cpp */
extern CFrontend *frontend;
extern XMLTreeParser *scanInputParser;
extern std::map <uint8_t, std::string> scanProviders;

int get_nits (uint32_t frequency, uint32_t symbol_rate, CodeRate FEC_inner, uint8_t polarity, uint8_t DiSEqC, Modulation modulation)
{
	FrontendParameters feparams;
	feparams.Frequency = frequency;
	feparams.Inversion = INVERSION_AUTO;

	if (frontend->getInfo()->type == FE_QPSK)
	{
		feparams.u.qpsk.SymbolRate = symbol_rate;
		feparams.u.qpsk.FEC_inner = FEC_inner;
	}
	else
	{
		feparams.u.qam.SymbolRate = symbol_rate;
		feparams.u.qam.FEC_inner = FEC_inner;
		feparams.u.qam.QAM = modulation;
	}

	if (frontend->tuneFrequency(feparams, polarity, DiSEqC) == true)
	{
		parse_nit(DiSEqC);
		return 0;
	}
	else
	{
		printf("No signal found on transponder\n");
		return -1;
	}
}

void get_sdts()
{
	stiterator tI;

	for (tI = scantransponders.begin(); tI != scantransponders.end(); tI++)
	{
		if (frontend->tuneFrequency(tI->second.feparams, tI->second.polarization, tI->second.DiSEqC) == true)
		{
			printf("[scan.cpp] parsing sdt of tsid %04x, onid %04x\n", tI->second.transport_stream_id, tI->second.original_network_id);
			parse_sdt(tI->second.transport_stream_id, true);
		}
		else
		{
			printf("[scan.cpp] No signal found on transponder\n");
		}
	}
}

FILE *write_xml_header (const char *filename)
{
	FILE *fd = fopen(filename, "w");

	if (fd == NULL)
	{
		perror("[scan.cpp] fopen");
		scan_runs = 0;
		pthread_exit(0);
	}
	else
	{
		fprintf(fd, "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n<zapit>\n");
	}

	return fd;
}

int write_xml_footer(FILE *fd)
{
	if (fd != NULL)
	{
		fprintf(fd, "</zapit>\n");
		return fclose(fd);
	}
	else
	{
		return -1;
	}
}

void write_bouquets(unsigned short mode)
{
	FILE *bouq_fd;
	std::string oldname = "";

	/*
	mode&1024 - loesche bouquets und erstelle sich nicht neu
	mode&512 - erstelle bouquets immer neu
	mode&256 - keine aenderung an bouqets
	*/

	if (mode & 1024)
	{
		printf("[zapit] removing existing bouqets.xml\n");
		system("/bin/rm " CONFIGDIR "/zapit/bouquets.xml");
		scanbouquets.clear();
		return;
	}

	if ((mode & 256) || (scanbouquets.empty()))
	{
		printf("[zapit] leavin bouquets.xml untouched\n");
		scanbouquets.clear();
		return;
	}
	else
	{
		printf("[zapit] creating new bouquets.xml\n");

		bouq_fd = write_xml_header(CONFIGDIR "/zapit/bouquets.xml");
		for (sbiterator bI = scanbouquets.begin(); bI != scanbouquets.end(); bI++)
      		{
      			if (bI->second.provname != oldname)
      			{
      				if (oldname != "")
      				{
      					fprintf(bouq_fd, "</Bouquet>\n");
      				}

      				fprintf(bouq_fd, "<Bouquet name=\"%s\">\n", bI->second.provname.c_str());

      			}
      			fprintf(bouq_fd, "\t<channel serviceID=\"%04x\" name=\"%s\" onid=\"%04x\"/>\n", bI->second.sid, bI->second.servname.c_str(), bI->second.onid);

      			oldname = bI->second.provname;
      		}

      		fprintf(bouq_fd, "</Bouquet>\n");
		write_xml_footer(bouq_fd);
      	}
      	scanbouquets.clear();
	return;
}

void write_transponder(FILE *fd, uint16_t transport_stream_id, uint16_t original_network_id, uint8_t diseqc)
{
	stiterator tI = scantransponders.find((transport_stream_id << 16) | original_network_id);

	/* cable */
	if (diseqc == 0xFF)
	{
		fprintf(fd,
			"\t\t<transponder id=\"%04x\" onid=\"%04x\" frequency=\"%u\" symbol_rate=\"%u\" fec_inner=\"%hhu\" modulation=\"%hhu\">\n",
			tI->second.transport_stream_id,
			tI->second.original_network_id,
			tI->second.feparams.Frequency,
			tI->second.feparams.u.qam.SymbolRate,
			tI->second.feparams.u.qam.FEC_inner,
			tI->second.feparams.u.qam.QAM);
	}

	/* satellite */
	else
	{
		fprintf(fd,
			"\t\t<transponder id=\"%04x\" onid=\"%04x\" frequency=\"%u\" symbol_rate=\"%u\" fec_inner=\"%hhu\" polarization=\"%hhu\">\n",
			tI->second.transport_stream_id,
			tI->second.original_network_id,
			tI->second.feparams.Frequency,
			tI->second.feparams.u.qpsk.SymbolRate,
			tI->second.feparams.u.qpsk.FEC_inner,
			tI->second.polarization);
	}

	for (sciterator cI = scanchannels.begin(); cI != scanchannels.end(); cI++)
	{
		if ((cI->second.tsid == transport_stream_id) && (cI->second.onid == original_network_id))
		{
			if (cI->second.name.length() == 0)
			{
				fprintf(fd,
					"\t\t\t<channel service_id=\"%04x\" name=\"%04x\" service_type=\"%04x\" channel_nr=\"0\"/>\n",
					cI->second.sid,
					cI->second.sid,
					cI->second.service_type);
			}
			else
			{
				fprintf(fd,
					"\t\t\t<channel service_id=\"%04x\" name=\"%s\" service_type=\"%04x\" channel_nr=\"0\"/>\n",
					cI->second.sid,
					cI->second.name.c_str(),
					cI->second.service_type);
			}
		}
	}

	fprintf(fd, "\t\t</transponder>\n");

	return;
}

FILE *write_provider(FILE *fd, const char *type, const char *provider_name, const uint8_t DiSEqC)
{
	if (!scantransponders.empty())
	{
		/* create new file if needed */
		if (fd == NULL)
		{
			fd = write_xml_header(CONFIGDIR "/services.xml");
		}

		/* cable tag */
		if (!strcmp(type, "cable"))
		{
			fprintf(fd, "\t<%s name=\"%s\">\n", type, provider_name);
		}

		/* satellite tag */
		else
		{
			fprintf(fd, "\t<%s name=\"%s\" diseqc=\"%hhd\">\n", type, provider_name, DiSEqC);
		}

		/* channels */
		for (stiterator tI = scantransponders.begin(); tI != scantransponders.end(); tI++)
		{
			write_transponder(fd, tI->second.transport_stream_id, tI->second.original_network_id, DiSEqC);
		}

		/* end tag */
		fprintf(fd, "\t</%s>\n", type);
	}

	/* clear results for next provider */
	scanchannels.clear();
	scantransponders.clear();

	return fd;
}

void stop_scan()
{
	/* notify client about end of scan */
	scan_runs = 0;
	eventServer->sendEvent(CZapitClient::EVT_SCAN_COMPLETE, CEventServer::INITID_ZAPIT);
}

void *start_scanthread(void *param)
{
	FILE *fd = NULL;

	char providerName[32];
	char type[8];

	/* still used for bouquets */
	unsigned short do_diseqc = *(unsigned short *) (param);

	uint8_t diseqc_pos = 0;

	uint32_t frequency;
	uint32_t symbol_rate;
	uint8_t polarization;
	uint8_t fec_inner;
	uint8_t modulation;

	curr_sat = 0;

	if ((frontend == NULL) || (frontend->isInitialized() == false))
	{
		printf("[scan.cpp] unable not scan without a frontend \n");
		stop_scan();
		pthread_exit(0);
	}

	switch (frontend->getInfo()->type)
	{
	case FE_QPSK:	/* satellite frontend */
		strcpy(type, "sat");
		modulation = 0;
		break;

	case FE_QAM:	/* cable frontend */
		strcpy(type, "cable");
		polarization = 0;
		break;
	
	default:	/* unsupported frontend */
		stop_scan();
		pthread_exit(0);
	}

	/* get first child */
	XMLTreeNode *search = scanInputParser->RootNode()->GetChild();
	XMLTreeNode *transponder = NULL;

	std::map <uint8_t, std::string>::iterator spI;

	/* notify client about start */
	scan_runs = 1;

	/* read all sat or cable sections */
	while ((search) && (!strcmp(search->GetType(), type)))
	{
		/* get name of current satellite oder cable provider */
		strcpy(providerName, search->GetAttributeValue("name"));

		/* look whether provider is wanted */
		for (spI = scanProviders.begin(); spI != scanProviders.end(); spI++)
		{
			if (!strcmp(spI->second.c_str(), providerName))
			{
				break;
			}
		}

		/* provider is not wanted - jump to the next one */
		if (spI == scanProviders.end())
		{
			search = search->GetNext();
			continue;
		}

		/* increase sat counter */
		curr_sat++;

		/* satellite tuners might need diseqc */
		if (frontend->getInfo()->type == FE_QPSK)
		{
			diseqc_pos = spI->first;
		}

		/* send sat name to client */
		eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, &providerName, strlen(providerName) + 1);
		transponder = search->GetChild();

		/* read all transponders */
		while ((transponder) && (!strcmp(transponder->GetType(), "transponder")))
		{
			/* generic */
			sscanf(transponder->GetAttributeValue("frequency"), "%u", &frequency);
			sscanf(transponder->GetAttributeValue("symbol_rate"), "%u", &symbol_rate);
			sscanf(transponder->GetAttributeValue("fec_inner"), "%hhu", &fec_inner);

			/* cable */
			if (frontend->getInfo()->type == FE_QAM)
			{
				sscanf(transponder->GetAttributeValue("modulation"), "%hhu", &modulation);
			}

			/* satellite */
			else
			{
				sscanf(transponder->GetAttributeValue("polarization"), "%hhu", &polarization);
			}

			/* read network information table */
			get_nits(frequency, symbol_rate, getFEC(fec_inner), polarization, diseqc_pos, getModulation(modulation));

			/* next transponder */
			transponder = transponder->GetNext();
		}

		/* read service description table */
		get_sdts();

		/* write services */
		fd = write_provider(fd, type, providerName, diseqc_pos);

		/* go to next satellite */
		search = search->GetNext();
	}

	/* clean up - should this be done before every GetNext() ? */
	delete transponder;
	delete search;

	/* close xml tags */
	if (write_xml_footer(fd) != -1)
	{
		/* write bouquets if channels did not fail */
		write_bouquets(do_diseqc);
	}

	/* report status */
	printf("[scan.cpp] found %d transponders and %d channels\n", found_transponders, found_channels);

	/* load new services */
	if (prepare_channels() < 0)
	{
		printf("[scan.cpp] Error parsing Services\n");
		stop_scan();
		pthread_exit(0);
	}
	else
	{
		printf("[scan.cpp] Channels have been loaded succesfully\n");
	}

	stop_scan();
	pthread_exit(0);
}

