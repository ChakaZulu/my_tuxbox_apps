/*
$Id: dvb_str.c,v 1.4 2001/10/06 18:19:18 Toerli Exp $

  -- dvb decoder helper functions


$Log: dvb_str.c,v $
Revision 1.4  2001/10/06 18:19:18  Toerli
Steuerzeichen entfernt. rasc wuerdest du mal bitte nen gescheiten unix-konformen Editor verwenden... windows editoren sind ungeeignet

Revision 1.3  2001/10/05 17:43:37  rasc
typo...

Revision 1.2  2001/10/02 21:52:44  rasc
- init der time_delta
- PES erweitert, PES arbeitet im read() noch nicht richtig!!
- muss tmbinc fragem, ob ich Mist baue, oder der Treiber (??)

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/



#include "dvbsnoop.h"
#include "dvb_str.h"



typedef struct _STR_TABLE {
    u_int    from;          /* e.g. from id 1  */
    u_int    to;            /*      to   id 3  */
    u_char   *str;          /*      is   string xxx */
} STR_TABLE;




/*
  -- match id in range from STR_TABLE
*/

static char *findTableID (STR_TABLE *t, u_int id)

{

  while (t->str) {
    if (t->from <= id && t->to >= id)
       return t->str;
    t++;
  }

  return ">>ERROR: not (yet) defined... Report!<<";
}





/* -----------------------------------------  */



/*
  --  Table IDs (stream)
 ETSI EN 468   5.2
*/

char *dvbstrTableID (u_int id)

{
  STR_TABLE  TableIDs[] = {

     {  0x00, 0x00,  "program_association_section" },
     {  0x01, 0x01,  "conditional_access_section" },
     {  0x02, 0x02,  "program_map_section" },
     {  0x03, 0x03,  "transport_stream_description_section" },
     {  0x04, 0x3F,  "ITU-T Rec. H.222.0|ISO/IEC13813 reserved" },

     {  0x40, 0x40,  "network_information_section - actual network" },
     {  0x41, 0x41,  "network_information_section - other network" },
     {  0x42, 0x42,  "service_description_section - actual transport stream" },
     {  0x43, 0x45,  "reserved" },
     {  0x46, 0x46,  "service_description_section - actual transport stream" },
     {  0x47, 0x49,  "reserved" },
     {  0x4A, 0x4A,  "bouquet_association_section" },
     {  0x4B, 0x4D,  "reserved" },
     {  0x4E, 0x4E,  "event_information_section - actual transport stream, present/following" },
     {  0x4F, 0x4F,  "event_information_section - other transport stream, present/following" },
     {  0x50, 0x5F,  "event_information_section - actual transport stream, schedule" },
     {  0x60, 0x6F,  "event_information_section - other transport stream, schedule" },
     {  0x70, 0x70,  "time_date_section" },
     {  0x71, 0x71,  "running_status_section" },
     {  0x72, 0x72,  "stuffing_section" },
     {  0x73, 0x73,  "time_offset_section" },
     {  0x74, 0x7D,  "reserved" },
     {  0x7E, 0x7E,  "discontinuity_information_section" },
     {  0x7F, 0x7F,  "selection_information_section" },
     {  0x80, 0xFE,  "User private" },
     {  0xFF, 0xFF,  "forbidden" },
     {  0,0, NULL }
  };


  return findTableID (TableIDs, id);
}


/*
  -- Descriptor table tags
*/

char *dvbstrDescriptorTAG (u_int tag)

{
  STR_TABLE  Tags[] = {
// ISO 13818-1
     {  0x00, 0x01,  "Reserved" },
     {  0x02, 0x02,  "video_stream_descriptor" },
     {  0x03, 0x03,  "audio_stream_descriptor" },
     {  0x04, 0x04,  "hierarchy_descriptor" },
     {  0x05, 0x05,  "registration_descriptor" },
     {  0x06, 0x06,  "data_stream_alignment_descriptor" },
     {  0x07, 0x07,  "target_background_grid_descriptor" },
     {  0x08, 0x08,  "videa_window_descriptor" },
     {  0x09, 0x09,  "CA_descriptor" },
     {  0x0A, 0x0A,  "ISO_639_language_descriptor" },
     {  0x0B, 0x0B,  "system_clock_descriptor" },
     {  0x0C, 0x0C,  "multiplex_buffer_utilization_descriptor" },
     {  0x0D, 0x0D,  "copyright_descriptor" },
     {  0x0E, 0x0E,  "maximum_bitrate_descriptor" },
     {  0x0F, 0x0F,  "private_data_indicator_descriptor" },
     {  0x10, 0x10,  "smoothing_buffer_descriptor" },
     {  0x11, 0x11,  "STD_descriptor" },
     {  0x12, 0x12,  "IBP_descriptor" },
     {  0x13, 0x3F,  "ITU-T.Rec.H.222.0|ISO/IEC13818-1 Reserved" },

// ETSI 300 468
     {  0x40, 0x40,  "network_name_descriptor" },
     {  0x41, 0x41,  "service_list_descriptor" },
     {  0x42, 0x42,  "stuffing_descriptor" },
     {  0x43, 0x43,  "satellite_delivery_system_descriptor" },
     {  0x44, 0x44,  "cable_delivery_system_descriptor" },
     {  0x45, 0x45,  "VBI_data_descriptor" },
     {  0x46, 0x46,  "VBI_teletext_descriptor" },
     {  0x47, 0x47,  "bouquet_name_descriptor" },
     {  0x48, 0x48,  "service_descriptor" },
     {  0x49, 0x49,  "country_availibility_descriptor" },
     {  0x4A, 0x4A,  "linkage_descriptor" },
     {  0x4B, 0x4B,  "NVOD_reference_descriptor" },
     {  0x4C, 0x4C,  "time_shifted_service_descriptor" },
     {  0x4D, 0x4D,  "short_event_descriptor" },
     {  0x4E, 0x4E,  "extended_event_descriptor" },
     {  0x4F, 0x4F,  "time_shifted_event_descriptor" },
     {  0x50, 0x50,  "component_descriptor" },
     {  0x51, 0x51,  "mosaic_descriptor" },
     {  0x52, 0x52,  "stream_identifier_descriptor" },
     {  0x53, 0x53,  "CA_identifier_descriptor" },
     {  0x54, 0x54,  "content_descriptor" },
     {  0x55, 0x55,  "parental_rating_descriptor" },
     {  0x56, 0x56,  "teletext_descriptor" },
     {  0x57, 0x57,  "telephone_descriptor" },
     {  0x58, 0x58,  "local_time_offset_descriptor" },
     {  0x59, 0x59,  "subtitling_descriptor" },
     {  0x5A, 0x5A,  "terrestrial_delivery_system_descriptor" },
     {  0x5B, 0x5B,  "multilingual_network_name_descriptor" },
     {  0x5C, 0x5C,  "multilingual_bouquet_name_descriptor" },
     {  0x5D, 0x5D,  "multilingual_service_name_descriptor" },
     {  0x5E, 0x5E,  "multilingual_component_descriptor" },
     {  0x5F, 0x5F,  "private_data_specifier_descriptor" },
     {  0x60, 0x60,  "service_move_descriptor" },
     {  0x61, 0x61,  "short_smoothing_buffer_descriptor" },
     {  0x62, 0x62,  "frequency_list_descriptor" },
     {  0x63, 0x63,  "partial_transport_stream_descriptor" },
     {  0x64, 0x64,  "data_broadcast_descriptor" },
     {  0x65, 0x65,  "CA_system_descriptor" },
     {  0x66, 0x66,  "data_broadcast_id_descriptor" },
     {  0x67, 0x67,  "transport_stream_descriptor" },
     {  0x68, 0x68,  "DSNG_descriptor" },
     {  0x69, 0x69,  "PDC_descriptor" },
     {  0x6A, 0x6A,  "AC3_descriptor" },
     {  0x6B, 0x6B,  "ancillary_data_descriptor" },
     {  0x6C, 0x6C,  "cell_list_descriptor" },
     {  0x6D, 0x6D,  "cell_frequency_list_descriptor" },
     {  0x6E, 0x6E,  "announcement_support_descriptor" },
     {  0x6F, 0x7F,  "reserved_descriptor" },
     {  0x80, 0xFE,  "User defined" },

     {  0xFF, 0xFF,  "Forbidden" },
     {  0,0, NULL }
  };


  return findTableID (Tags, tag);
}



/*
 -- delivery desctritor flags
 -- ETSI EN 468 6.2.12.1 ff
*/

char *dvbstrWEST_EAST_FLAG (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "WEST" },
     {  0x01, 0x01,  "EAST" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}



char *dvbstrPolarisation_FLAG (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "linear - horizontal" },
     {  0x01, 0x01,  "linear - vertical" },
     {  0x02, 0x02,  "circular - left" },
     {  0x03, 0x03,  "circular - right" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}


char *dvbstrModulationSAT_FLAG (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "not defined" },
     {  0x01, 0x01,  "QPSK" },
     {  0x02, 0x1F,  "reserved for future use" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}



char *dvbstrModulationCable_FLAG (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "not defined" },
     {  0x01, 0x01,  "16 QAM" },
     {  0x02, 0x02,  "32 QAM" },
     {  0x03, 0x03,  "64 QAM" },
     {  0x04, 0x04,  "128 QAM" },
     {  0x05, 0x05,  "256 QAM" },
     {  0x06, 0xFF,  "reserved for future use" },
     {  0,0, NULL }
  };


  return findTableID (Table, flag);
}



char *dvbstrFECinner_SCHEME (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "not defined" },
     {  0x01, 0x01,  "1/2 conv. code rate" },
     {  0x02, 0x02,  "2/3 conv. code rate" },
     {  0x03, 0x03,  "3/4 conv. code rate" },
     {  0x04, 0x04,  "5/6 conv. code rate" },
     {  0x05, 0x05,  "7/8 conv. code rate" },
     {  0x06, 0x0E,  "reserved" },
     {  0x0F, 0x0F,  "No conv. coding" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}



char *dvbstrFECouter_SCHEME (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "not defined" },
     {  0x01, 0x01,  "no outer FEC coding" },
     {  0x02, 0x02,  "RS(204/188)" },
     {  0x03, 0x0F,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}





/*
  -- Linkage type descriptor
*/

char *dvbstrLinkage_TYPE (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "information service" },
     {  0x02, 0x02,  "EPG service" },
     {  0x03, 0x03,  "CA replacement service" },
     {  0x04, 0x04,  "TS containing complete Network/Bouquet SI" },
     {  0x05, 0x05,  "service replacement service" },
     {  0x06, 0x06,  "data broadcast service" },
     {  0x07, 0x07,  "RCS Map" },
     {  0x08, 0x08,  "mobile handover service" },
     {  0x09, 0x7F,  "reserved" },
     {  0x80, 0xFE,  "user defined" },
     {  0xFF, 0xFF,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}



char *dvbstrHandover_TYPE (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "DVB hand-over to an identical service in a neighbouring country" },
     {  0x02, 0x02,  "DVB hand-over to local variation to same service" },
     {  0x02, 0x02,  "DVB hand-over to an associated service" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}



char *dvbstrOrigin_TYPE (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "NIT" },
     {  0x01, 0x01,  "SDT" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}





/*
 -- Service Link Descriptor
*/ 

char *dvbstrService_TYPE (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "digital television service" },
     {  0x02, 0x02,  "digital radio sound service" },
     {  0x03, 0x03,  "Teletext service" },
     {  0x04, 0x04,  "NVOD reference service" },
     {  0x05, 0x05,  "NVOD time-shifted service" },
     {  0x06, 0x06,  "mosaic service" },
     {  0x07, 0x07,  "PAL coded signal" },
     {  0x08, 0x08,  "SECAM coded signal" },
     {  0x09, 0x09,  "D/D2-MAC" },
     {  0x0A, 0x0A,  "FM-Radio" },
     {  0x0B, 0x0B,  "NTSC coded signal" },
     {  0x0C, 0x0C,  "data broadcast service" },
     {  0x0D, 0x0D,  "reserved for Common Interface Usage" },
     {  0x0E, 0x0E,  "RCS Map" },
     {  0x0F, 0x0F,  "RCS FLS" },
     {  0x10, 0x10,  "DVB  MHP service" },
     {  0x11, 0x7F,  "reserved" },
     {  0x80, 0xFE,  "User defined" },
     {  0xFF, 0xFF,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}



/*
 -- Programm Map Table   Stream Type
*/

char *dvbstrStream_TYPE (u_int flag)

{
  /* ISO 13818-1  Table 2.36  */

  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "ITU-T | ISO-IE Reserved" },
     {  0x01, 0x01,  "ISO/IEC 11172 Video" },
     {  0x02, 0x02,  "ITU-T Rec. H.262 | ISO/IEC 13818-2 Video | ISO/IEC 11172-2 constr. parameter video stream" },
     {  0x03, 0x03,  "ISO/IEC 11172 Audio" },
     {  0x04, 0x04,  "ISO/IEC 13818-3 Audio" },
     {  0x05, 0x05,  "ITU-T Rec. H.222.0 | ISO/IEC 13818-1 private sections" },
     {  0x06, 0x06,  "ITU-T Rec. H.222.0 | ISO/IEC 13818-1 PES packets containing private data" },
     {  0x07, 0x07,  "ISO/IEC 13512 MHEG" },
     {  0x08, 0x08,  "ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Annex A  DSM CC" },
     {  0x09, 0x09,  "ITU-T Rec. H.222.1" },
     {  0x0A, 0x0A,  "ISO/IEC 13818-6 Type A" },
     {  0x0B, 0x0B,  "ISO/IEC 13818-6 Type B" },
     {  0x0C, 0x0C,  "ISO/IEC 13818-6 Type C" },
     {  0x0D, 0x0D,  "ISO/IEC 13818-6 Type D" },
     {  0x0E, 0x0E,  "ISO/IEC 13818-1 auxiliary" },
     {  0x0F, 0x7F,  "ITU-T Rec. H.222.0 | ISO/IEC 13818-1 reserved" },
     {  0x80, 0xFF,  "User private" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}



/*
 -- Audio Types (descriptor e.g. ISO 639)
*/

char *dvbstrAudio_TYPE (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "undefined" },
     {  0x01, 0x01,  "clean effects" },
     {  0x02, 0x02,  "hearing impaired" },
     {  0x03, 0x03,  "visual impaired commentary" },
     {  0x04, 0xFF,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}





/*
 -- CA-System Identifier  (ETSI ETR 162)
*/

char *dvbstrCASystem_ID (u_int id)

{
  STR_TABLE  Table[] = {
     {  0x0000, 0x0000,  "Reserved" },
     {  0x0001, 0x00FF,  "Standardized Systems" },
     {  0x0100, 0x01FF,  "Canal Plus (Seca/MediaGuard)" },
     {  0x0200, 0x02FF,  "CCETT" },
     {  0x0300, 0x03FF,  "Deutsche Telekom" },
     {  0x0400, 0x04FF,  "Eurodec" },
     {  0x0500, 0x05FF,  "France Telecom (Viaccess)" },
     {  0x0600, 0x06FF,  "Irdeto" },
     {  0x0700, 0x07FF,  "Jerrold/GI" },
     {  0x0800, 0x08FF,  "Matra Communication" },
     {  0x0900, 0x09FF,  "News Datacomi (Videoguard)" },
     {  0x0A00, 0x0AFF,  "Nokia" },
     {  0x0B00, 0x0BFF,  "Norwegian Telekom (Conax)" },
     {  0x0C00, 0x0CFF,  "NTL" },
     {  0x0D00, 0x0DFF,  "Philips (Cryptoworks)" },
     {  0x0E00, 0x0EFF,  "Scientific Atlanta (Power VU)" },
     {  0x0F00, 0x0FFF,  "Sony" },
     {  0x1000, 0x10FF,  "Tandberg Television" },
     {  0x1100, 0x11FF,  "Thompson" },
     {  0x1200, 0x12FF,  "TV/COM" },
     {  0x1300, 0x13FF,  "HPT - Croatian Post and Telecommunications" },
     {  0x1400, 0x14FF,  "HRT - Croatian Radio and Television" },
     {  0x1500, 0x15FF,  "IBM" },
     {  0x1600, 0x16FF,  "Nera" },
     {  0x1700, 0x17FF,  "Beta Technik (Betacrypt)" },
//$$$ some are missing and have to be hijacked from e.g. tmbinc
     {  0,0, NULL }
  };

  return findTableID (Table, id);
}






/*
 -- Network Identification coding (ETR 162)
*/

char *dvbstrNetworkIdent_ID (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x0000, 0x0000,  "reserved" },
     {  0x0001, 0x0001,  "Astra Satellite Network 19.2E / Satellite / SES" },
     {  0x0027, 0x0028,  "Hispasat 30W / Satellite / Hispasat FSS" },
     {  0x0028, 0x0028,  "Hispasat 30W / Satellite / Hispasat DBS" },
     {  0x0029, 0x0029,  "Hispasat 30W / Satellite / Hispasat America" },
     {  0x0085, 0x0085,  "- / Satellite / Beta Technik" },
     {  0x013E, 0x013E,  "Eutel Satellite System 13.0E / Satellite / ETSO" },
//$$$ lots are missing
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}




/*
 -- Teletext type descriptor (ETSI EN 300 468  6.2.38)
*/

char *dvbstrTeletext_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "initial teletext page" },
     {  0x02, 0x02,  "teletext subtitle page" },
     {  0x03, 0x03,  "additional information page" },
     {  0x04, 0x04,  "program schedule page" },
     {  0x05, 0x05,  "teletext subtitle page for hearing impaired people" },
     {  0x06, 0x1F,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}




/*
 -- Terrestrial Bandwidth descriptor (ETSI EN 300 468  6.2.12.3)
*/

char *dvbstrTerrBandwidth_SCHEME (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "8 MHz" },
     {  0x01, 0x01,  "7 MHz" },
     {  0x02, 0x02,  "6 MHz" },
     {  0x03, 0x07,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}


char *dvbstrTerrConstellation_FLAG (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "QPSK" },
     {  0x01, 0x01,  "16-QAM" },
     {  0x02, 0x02,  "64-QAM" },
     {  0x03, 0x03,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}


char *dvbstrTerrHierarchy_FLAG (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "non-hierarchical" },
     {  0x01, 0x01,  "alpha=1" },
     {  0x02, 0x02,  "alpha=2" },
     {  0x03, 0x03,  "alpha=4" },
     {  0x04, 0x07,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}


char *dvbstrTerrCodeRate_FLAG (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "1/2" },
     {  0x01, 0x01,  "2/3" },
     {  0x02, 0x02,  "3/4" },
     {  0x03, 0x03,  "5/6" },
     {  0x04, 0x04,  "7/8" },
     {  0x05, 0x07,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}


char *dvbstrTerrGuardInterval_FLAG (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "1/32" },
     {  0x01, 0x01,  "1/16" },
     {  0x02, 0x02,  "1/8" },
     {  0x03, 0x03,  "1/4" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}


char *dvbstrTerrTransmissionMode_FLAG (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "2k mode" },
     {  0x01, 0x01,  "8k mode" },
     {  0x02, 0x03,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}


/*
 -- Aspect Ratio  (e.g. Target Background Grid)
 -- ISO 13818-2  Table 6.3
*/

char *dvbstrAspectRatioInfo_FLAG (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "forbidden" },
     {  0x01, 0x01,  " -- " },
     {  0x02, 0x02,  "3:4" },
     {  0x03, 0x03,  "9:16" },
     {  0x04, 0x04,  "1:2.21" },
     {  0x05, 0x0F,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}



/*
 -- Hierarchy Type  
 -- ISO 13818-1  Table 2.6.7
*/

char *dvbstrHierarchy_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "ITU-T Rec.H.262 | ISO/IEC 13818-2 Spatial Scalability" },
     {  0x02, 0x02,  "ITU-T Rec.H.262 | ISO/IEC 13818-2 SNR Scalability" },
     {  0x03, 0x03,  "ITU-T Rec.H.262 | ISO/IEC 13818-2 Temporal Scalability" },
     {  0x04, 0x04,  "ITU-T Rec.H.262 | ISO/IEC 13818-2 Data partioning" },
     {  0x05, 0x05,  "ISO/IEC 13818-3 Extension bitstream" },
     {  0x06, 0x06,  "ITU-T Rec.H.222.0 | ISO/IEC 13818-1 Private Stream" },
     {  0x07, 0x0E,  "reserved" },
     {  0x0F, 0x0F,  "Base layer" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}




/*
 -- Running Status  (SDT)  
 -- ETSI EN 300 468   5.2.3
*/

char *dvbstrRunningStatus_FLAG (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "undefined" },
     {  0x01, 0x01,  "not running" },
     {  0x02, 0x02,  "starts in a few seconds (e.g. for VCR)" },
     {  0x03, 0x03,  "pausing" },
     {  0x04, 0x04,  "running" },
     {  0x05, 0x07,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}




/*
 -- Data Stream Alignment Type
 -- ISO 13818-1  2.6.11
*/

char *dvbstrDataStreamVIDEOAlignment_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "Slice or video access unit" },
     {  0x02, 0x02,  "video access unit" },
     {  0x03, 0x03,  "GOP or SEQ" },
     {  0x04, 0x04,  "SEQ" },
     {  0x05, 0xFF,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}


char *dvbstrDataStreamAUDIOAlignment_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "syncword" },
     {  0x02, 0xFF,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}



/*
  -- VBI Data Service ID
  -- ETSI EN 300 468   6.2.43
*/

char *dvbstrDataService_ID (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "EBU teletext" },
     {  0x02, 0x02,  "inverted teletext" },
     {  0x03, 0x03,  "reserved" },
     {  0x04, 0x04,  "VPS" },
     {  0x05, 0x05,  "WSS" },
     {  0x06, 0x06,  "Closed Caption" },
     {  0x07, 0x07,  "monochrome 4:2:2 samples" },
     {  0x08, 0xFF,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}





/*
  -- Stream Content & Component Type
  -- ETSI EN 300 468   6.2.7
*/

char *dvbstrStreamContent_Component_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     // streamComponentID << 8    | ComponentID
     {  0x0000, 0x00FF,  "reserved" },
     {  0x0100, 0x0100,  "reserved" },

     {  0x0101, 0x0101,  "video, 4:3  aspect ratio, 25 Hz" },
     {  0x0102, 0x0102,  "video, 16:9 aspect ratio with pan vectors, 25 Hz" },
     {  0x0103, 0x0103,  "video, 16:9 aspect ratio without pan vectors, 25 Hz" },
     {  0x0104, 0x0104,  "video, > 16:9 aspect ratio, 25 Hz" },
     {  0x0105, 0x0105,  "video, 4:3  aspect ratio, 30 Hz" },
     {  0x0106, 0x0106,  "video, 16:9 aspect ratio with pan vectors, 30 Hz" },
     {  0x0107, 0x0107,  "video, 16:9 aspect ratio without pan vectors, 30 Hz" },
     {  0x0108, 0x0108,  "video, > 16:9 aspect ratio, 30 Hz" },

     {  0x0109, 0x0109,  "high definition video, 4:3  aspect ratio, 25 Hz" },
     {  0x010A, 0x010A,  "high definition video, 16:9 aspect ratio with pan vectors, 25 Hz" },
     {  0x010B, 0x010B,  "high definition video, 16:9 aspect ratio without pan vectors, 25 Hz" },
     {  0x010C, 0x010C,  "high definition video, > 16:9 aspect ratio, 25 Hz" },
     {  0x010D, 0x010D,  "high definition video, 4:3  aspect ratio, 30 Hz" },
     {  0x010E, 0x010E,  "high definition video, 16:9 aspect ratio with pan vectors, 30 Hz" },
     {  0x010F, 0x010F,  "high definition video, 16:9 aspect ratio without pan vectors, 30 Hz" },
     {  0x0110, 0x0110,  "high definition video, > 16:9 aspect ratio, 30 Hz" },

     {  0x0111, 0x01AF,  "reserved" },
     {  0x01B0, 0x01FE,  "User defined" },
     {  0x01FF, 0x01FF,  "reserved" },
     {  0x0200, 0x0200,  "reserved" },

     {  0x0201, 0x0201,  "audio, single mono channel" },
     {  0x0202, 0x0202,  "audio, dual mono channel" },
     {  0x0203, 0x0203,  "audio, stereo (2 channels)" },
     {  0x0204, 0x0204,  "audio, multilingual, multi-channel)" },
     {  0x0205, 0x0205,  "audio, surround sound" },
     {  0x0206, 0x023F,  "reserved" },
     {  0x0240, 0x0240,  "audio description for visually impaired" },
     {  0x0241, 0x0241,  "audio for the hard of hearing" },

     {  0x0242, 0x02AF,  "reserved" },
     {  0x02B0, 0x02FE,  "User defined" },
     {  0x02FF, 0x02FF,  "reserved" },
     {  0x0300, 0x0300,  "reserved" },

     {  0x0301, 0x0301,  "EBU Teletext subtitles" },
     {  0x0302, 0x0302,  "associated Teletext" },
     {  0x0303, 0x0303,  "VBI data" },
     {  0x0304, 0x030F,  "reserved" },

     {  0x0310, 0x0310,  "DVB subtitles (normal) with no monitor aspect ratio critical" },
     {  0x0311, 0x0311,  "DVB subtitles (normal) for display 4:3 aspect ratio monitor" },
     {  0x0312, 0x0312,  "DVB subtitles (normal) for display 16:9 aspect ratio monitor" },
     {  0x0313, 0x0313,  "DVB subtitles (normal) for display 2.21:1 aspect ratio monitor" },
     {  0x0314, 0x031F,  "reserved" },
     {  0x0320, 0x0320,  "DVB subtitles (for the hard hearing) with no monitor aspect ratio critical" },
     {  0x0321, 0x0321,  "DVB subtitles (for the hard hearing) for display 4:3 aspect ratio monitor" },
     {  0x0322, 0x0322,  "DVB subtitles (for the hard hearing) for display 16:9 aspect ratio monitor" },
     {  0x0323, 0x0323,  "DVB subtitles (for the hard hearing) for display 2.21:1 aspect ratio monitor" },

     {  0x0324, 0x03AF,  "reserved" },
     {  0x03B0, 0x03FE,  "User defined" },
     {  0x03FF, 0x03FF,  "reserved" },
     {  0x0400, 0x0400,  "reserved" },
     {  0x0401, 0x047F,  "AC3 modes  (ERROR: to be defined more specific $$$)" },

     {  0x0480, 0x04FF,  "reserved" },
     {  0x0500, 0x0BFF,  "reserved" },
     {  0x0C00, 0x0FFF,  "User defined" },

     {  0,0, NULL }
  };

  return findTableID (Table, i);
}




/*
  -- Logical Cell Presentation Info
  -- ETSI EN 300 468   6.2.18
*/

char *dvbstrLogCellPresInfo_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "undefined" },
     {  0x01, 0x01,  "video" },
     {  0x02, 0x02,  "still picture (INTRA coded)" },
     {  0x03, 0x03,  "graphics/text" },
     {  0x04, 0x07,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}



/*
  -- Cell Linkage Info
  -- ETSI EN 300 468   6.2.18
*/

char *dvbstrCellLinkageInfo_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "undefined" },
     {  0x01, 0x01,  "bouquet related" },
     {  0x02, 0x02,  "service related" },
     {  0x03, 0x03,  "other mosaic related" },
     {  0x04, 0x04,  "event related" },
     {  0x05, 0xFF,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}



/*
  -- Text Charset Types
  -- ETSI EN 300 468   ANNEX A
*/

char *dvbstrTextCharset_TYPE(u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "Latin/Cyrilic alphabet" },
     {  0x02, 0x02,  "Latin/Arabic alphabet" },
     {  0x03, 0x03,  "Latin/Greek alphabet" },
     {  0x04, 0x04,  "Latin/Hebrew alphabet" },
     {  0x05, 0x05,  "Latin alphabet no. 5" },
     {  0x06, 0x0F,  "reserved" },
     {  0x10, 0x10,  "ISO/IEC 8859  special table " },
     {  0x11, 0x11,  "ISO/IEC 10646-1 2Byte pairs Basic Multilingual Plane" },
     {  0x12, 0x12,  "Korean Charset KSC 5601" },
     {  0x13, 0x1F,  "reserved" },
     {  0x20, 0xFF,  "Latin alphabet" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}



/*
  -- Content Nibble Types (Content descriptor)
  -- ETSI EN 300 468   6.2.8
*/

char *dvbstrContentNibble_TYPE(u_int i)

{
  STR_TABLE  Table[] = {
     // ContenNibble_1 << 8    |  ContentNibble_2
     //  4 bit                 |       4 bit
     {  0x0000, 0x000F,  "reserved" },

     // Movie/Drama
     {  0x0100, 0x0100,  "movie/drama (general)" },
     {  0x0101, 0x0101,  "detective/thriller" },
     {  0x0102, 0x0102,  "adventure/western/war" },
     {  0x0103, 0x0103,  "science fiction/fantasy/horror" },
     {  0x0104, 0x0104,  "comedy" },
     {  0x0105, 0x0105,  "soap/melodram/folkloric" },
     {  0x0106, 0x0106,  "romance" },
     {  0x0107, 0x0107,  "serious/classical/religious/historical movie/drama" },
     {  0x0108, 0x0108,  "adult movie/drama" },

     {  0x0109, 0x010E,  "reserved" },
     {  0x010F, 0x010A,  "user defined" },

     // News Current Affairs
     {  0x0200, 0x0200,  "news/current affairs (general)" },
     {  0x0201, 0x0201,  "news/weather report" },
     {  0x0202, 0x0202,  "news magazine" },
     {  0x0203, 0x0203,  "documentary" },
     {  0x0204, 0x0204,  "discussion/interview/debate" },
     {  0x0205, 0x020E,  "reserved" },
     {  0x020F, 0x020F,  "user defined" },

     // Show Games show
     {  0x0300, 0x0300,  "show/game show (general)" },
     {  0x0301, 0x0301,  "game show/quiz/contest" },
     {  0x0302, 0x0302,  "variety show" },
     {  0x0303, 0x0303,  "talk show" },
     {  0x0304, 0x030E,  "reserved" },
     {  0x030F, 0x030F,  "user defined" },

     // Sports
     {  0x0400, 0x0400,  "sports (general)" },
     {  0x0401, 0x0401,  "special events" },
     {  0x0402, 0x0402,  "sports magazine" },
     {  0x0403, 0x0403,  "football/soccer" },
     {  0x0404, 0x0404,  "tennis/squash" },
     {  0x0405, 0x0405,  "team sports" },
     {  0x0406, 0x0406,  "athletics" },
     {  0x0407, 0x0407,  "motor sport" },
     {  0x0408, 0x0408,  "water sport" },
     {  0x0409, 0x0409,  "winter sport" },
     {  0x040A, 0x040A,  "equestrian" },
     {  0x040B, 0x040B,  "martial sports" },
     {  0x040C, 0x040E,  "reserved" },
     {  0x040F, 0x040F,  "user defined" },

     // Children/Youth
     {  0x0500, 0x0500,  "childrens's/youth program (general)" },
     {  0x0501, 0x0501,  "pre-school children's program" },
     {  0x0502, 0x0502,  "entertainment (6-14 year old)" },
     {  0x0503, 0x0503,  "entertainment (10-16 year old)" },
     {  0x0504, 0x0504,  "information/education/school program" },
     {  0x0505, 0x0505,  "cartoon/puppets" },
     {  0x0506, 0x050E,  "reserved" },
     {  0x050F, 0x050F,  "user defined" },

     // Music/Ballet/Dance 
     {  0x0600, 0x0600,  "music/ballet/dance (general)" },
     {  0x0601, 0x0601,  "rock/pop" },
     {  0x0602, 0x0602,  "serious music/classic music" },
     {  0x0603, 0x0603,  "folk/traditional music" },
     {  0x0604, 0x0604,  "jazz" },
     {  0x0605, 0x0605,  "musical/opera" },
     {  0x0606, 0x0606,  "ballet" },
     {  0x0607, 0x060E,  "reserved" },
     {  0x060F, 0x060F,  "user defined" },

     // Arts/Culture
     {  0x0700, 0x0700,  "arts/culture (without music, general)" },
     {  0x0701, 0x0701,  "performing arts" },
     {  0x0702, 0x0702,  "fine arts" },
     {  0x0703, 0x0703,  "religion" },
     {  0x0704, 0x0704,  "popular culture/traditional arts" },
     {  0x0705, 0x0705,  "literature" },
     {  0x0706, 0x0706,  "film/cinema" },
     {  0x0707, 0x0707,  "experimental film/video" },
     {  0x0708, 0x0708,  "broadcasting/press" },
     {  0x0709, 0x0709,  "new media" },
     {  0x070A, 0x070A,  "arts/culture magazine" },
     {  0x070B, 0x070B,  "fashion" },
     {  0x070C, 0x070E,  "reserved" },
     {  0x070F, 0x070F,  "user defined" },

     // Social/Political/Economics
     {  0x0800, 0x0800,  "social/political issues/economics (general)" },
     {  0x0801, 0x0801,  "magazines/reports/documentary" },
     {  0x0802, 0x0802,  "economics/social advisory" },
     {  0x0803, 0x0803,  "remarkable people" },
     {  0x0804, 0x080E,  "reserved" },
     {  0x080F, 0x080F,  "user defined" },

     // Education/Science/...
     {  0x0900, 0x0900,  "education/science/factual topics (general)" },
     {  0x0901, 0x0901,  "nature/animals/environment" },
     {  0x0902, 0x0902,  "technology/natural science" },
     {  0x0903, 0x0903,  "medicine/physiology/psychology" },
     {  0x0904, 0x0904,  "foreign countries/expeditions" },
     {  0x0905, 0x0905,  "social/spiritual science" },
     {  0x0906, 0x0906,  "further education" },
     {  0x0907, 0x0907,  "languages" },
     {  0x0908, 0x090E,  "reserved" },
     {  0x090F, 0x090F,  "user defined" },

     // Leisure hobies
     {  0x0A00, 0x0A00,  "leisure hobbies (general)" },
     {  0x0A01, 0x0A01,  "tourism/travel" },
     {  0x0A02, 0x0A02,  "handicraft" },
     {  0x0A03, 0x0A03,  "motoring" },
     {  0x0A04, 0x0A04,  "fitness & health" },
     {  0x0A05, 0x0A05,  "cooking" },
     {  0x0A06, 0x0A06,  "advertisement/shopping" },
     {  0x0A07, 0x0A07,  "gardening" },
     {  0x0A08, 0x0A0E,  "reserved" },
     {  0x0A0F, 0x0A0F,  "user defined" },

     {  0x0B00, 0x0B00,  "original language" },
     {  0x0B01, 0x0B01,  "black & white" },
     {  0x0B02, 0x0B02,  "unpublished" },
     {  0x0B03, 0x0B03,  "live broadcast" },
     {  0x0B04, 0x0B0E,  "reserved" },
     {  0x0B0F, 0x0B0F,  "user defined" },

     {  0x0C00, 0x0E0F,  "reserved" },
     {  0x0F00, 0x0F0F,  "user defined" },

     {  0,0, NULL }
  };

  return findTableID (Table, i);
}



/*
  -- Parental Rating Info
  -- ETSI EN 300 468   6.2.25
*/

char *dvbstrParentalRating_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "undefined" },
     {  0x01, 0x01,  "minimum age: 4 years" },
     {  0x02, 0x02,  "minimum age: 5 years" },
     {  0x03, 0x03,  "minimum age: 6 years" },
     {  0x04, 0x04,  "minimum age: 7 years" },
     {  0x05, 0x05,  "minimum age: 8 years" },
     {  0x06, 0x06,  "minimum age: 9 years" },
     {  0x07, 0x07,  "minimum age: 10 years" },
     {  0x08, 0x08,  "minimum age: 11 years" },
     {  0x09, 0x09,  "minimum age: 12 years" },
     {  0x0A, 0x0A,  "minimum age: 13 years" },
     {  0x0B, 0x0B,  "minimum age: 14 years" },
     {  0x0C, 0x0C,  "minimum age: 15 years" },
     {  0x0D, 0x0D,  "minimum age: 16 years" },
     {  0x0E, 0x0E,  "minimum age: 17 years" },
     {  0x0F, 0x0F,  "minimum age: 18 years" },
     {  0x10, 0xFF,  "defined by broadcaster" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}




/*
  -- Delivery System Coding Type
  -- ETSI EN 300 468   6.2.15
*/

char *dvbstrDelivSysCoding_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "undefined" },
     {  0x01, 0x01,  "satellite" },
     {  0x02, 0x02,  "cable" },
     {  0x03, 0x03,  "terrestrial" },
     {  0,0, NULL }
  };


  return findTableID (Table, i);
}






/*
  -- Short Smoothing Buffer Size Type
  -- ETSI EN 300 468   6.2.29
*/

char *dvbstrShortSmoothingBufSize_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "1536 Bytes" },
     {  0x02, 0x02,  "reserved" },
     {  0x03, 0x03,  "reserved" },
     {  0,0, NULL }
  };


  return findTableID (Table, i);
}



char *dvbstrShortSmoothingBufLeakRate_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "0.0009 Mbit/s" },
     {  0x02, 0x02,  "0.0018 Mbit/s" },
     {  0x03, 0x03,  "0.0036 Mbit/s" },
     {  0x04, 0x04,  "0.0072 Mbit/s" },
     {  0x05, 0x05,  "0.0108 Mbit/s" },
     {  0x06, 0x06,  "0.0144 Mbit/s" },
     {  0x07, 0x07,  "0.0216 Mbit/s" },
     {  0x08, 0x08,  "0.0288 Mbit/s" },
     {  0x09, 0x09,  "0.075 Mbit/s" },
     {  0x0A, 0x0A,  "0.5 Mbit/s" },
     {  0x0B, 0x0B,  "0.5625 Mbit/s" },
     {  0x0C, 0x0C,  "0.8437 Mbit/s" },
     {  0x0D, 0x0D,  "1.0 Mbit/s" },
     {  0x0E, 0x0E,  "1.1250 Mbit/s" },
     {  0x0F, 0x0F,  "1.5 Mbit/s" },
     {  0x10, 0x10,  "1.6875 Mbit/s" },
     {  0x11, 0x11,  "2.0 Mbit/s" },
     {  0x12, 0x12,  "2.25 Mbit/s" },
     {  0x13, 0x13,  "2.5 Mbit/s" },
     {  0x14, 0x14,  "3.0 Mbit/s" },
     {  0x15, 0x15,  "3.3750 Mbit/s" },
     {  0x16, 0x16,  "3.5 Mbit/s" },
     {  0x17, 0x17,  "4.0 Mbit/s" },
     {  0x18, 0x18,  "4.5 Mbit/s" },
     {  0x19, 0x19,  "5.0 Mbit/s" },
     {  0x1A, 0x1A,  "5.5 Mbit/s" },
     {  0x1B, 0x1B,  "6.0 Mbit/s" },
     {  0x1C, 0x1C,  "6.5 Mbit/s" },
     {  0x1D, 0x1D,  "6.75 Mbit/s" },
     {  0x1E, 0x1E,  "7.0 Mbit/s" },
     {  0x1F, 0x1F,  "7.5 Mbit/s" },
     {  0x20, 0x20,  "8.0 Mbit/s" },
     {  0x21, 0x21,  "9 Mbit/s" },
     {  0x22, 0x22,  "10 Mbit/s" },
     {  0x23, 0x23,  "11 Mbit/s" },
     {  0x24, 0x24,  "12 Mbit/s" },
     {  0x25, 0x25,  "13 Mbit/s" },
     {  0x26, 0x26,  "13.5 Mbit/s" },
     {  0x27, 0x27,  "14.0 Mbit/s" },
     {  0x28, 0x28,  "15 Mbit/s" },
     {  0x29, 0x29,  "16 Mbit/s" },
     {  0x2A, 0x2A,  "17 Mbit/s" },
     {  0x2B, 0x2B,  "18 Mbit/s" },
     {  0x2C, 0x2C,  "20 Mbit/s" },
     {  0x2D, 0x2D,  "22 Mbit/s" },
     {  0x2E, 0x2E,  "24 Mbit/s" },
     {  0x2F, 0x2F,  "26 Mbit/s" },
     {  0x30, 0x30,  "27 Mbit/s" },
     {  0x31, 0x31,  "28 Mbit/s" },
     {  0x32, 0x32,  "30 Mbit/s" },
     {  0x33, 0x33,  "32 Mbit/s" },
     {  0x34, 0x34,  "34 Mbit/s" },
     {  0x35, 0x35,  "36 Mbit/s" },
     {  0x36, 0x36,  "38 Mbit/s" },
     {  0x37, 0x37,  "40 Mbit/s" },
     {  0x38, 0x38,  "44 Mbit/s" },
     {  0x39, 0x39,  "48 Mbit/s" },
     {  0x3A, 0x3A,  "54 Mbit/s" },
     {  0x3B, 0x3B,  "72 Mbit/s" },
     {  0x3C, 0x3C,  "108 Mbit/s" },
     {  0x3D, 0x3F,  "reserved" },
     {  0,0, NULL }
  };


  return findTableID (Table, i);
}





/*
  -- AC3 Component Type
  -- ETSI EN 300 468   ANNEX D
*/

char *dvbstrAC3Component_TYPE (u_int i)

{
  char *s = "ERROR:  TODO $$$  - AC3 Component type";



}



/*
  -- Ancillary Data ID
  -- ETSI EN 300 468   6.2.1
*/

char *dvbstrAncillaryData_ID (u_int i)

{

 // $$$ coded in descriptor



}



/*
  -- Announcement Type
  -- ETSI EN 300 468   6.2.2
*/

char *dvbstrAnnouncement_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "Emergency alarm" },
     {  0x01, 0x01,  "Road Traffic Flash" },
     {  0x02, 0x02,  "Public Transport Flash" },
     {  0x03, 0x03,  "Warning message" },
     {  0x04, 0x04,  "News flash" },
     {  0x05, 0x05,  "Weather flash" },
     {  0x06, 0x06,  "Event announcement" },
     {  0x07, 0x07,  "Personal call" },
     {  0x08, 0x0F,  "reserved" },
     {  0,0, NULL }
  };


  return findTableID (Table, i);
}



char *dvbstrAnnouncementReference_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "Announcement is broadcast in the usual audio stream of the service" },
     {  0x01, 0x01,  "Announcement is broadcast in the separate audio stream that is part of the service" },
     {  0x02, 0x02,  "Announcement is broadcast by means of a different service within the same transport stream" },
     {  0x03, 0x03,  "Announcement is broadcast by means of a different service within a different transport stream" },
     {  0x04, 0x0F,  "reserved" },
     {  0,0, NULL }
  };


  return findTableID (Table, i);
}





/*
 ------------------------------------------------------------------------
   Transport Stream  Stuff
 ------------------------------------------------------------------------
*/


/*
  -- Transport Stream PID  Table  ISO 13818-1  2.4.3.2
*/

char *dvbstrTSpid_ID (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x0000, 0x0000,  "Programm Association Table" },
     {  0x0001, 0x0001,  "Conditional Access Table" },
     {  0x0002, 0x000F,  "reserved" },
     {  0x0010, 0x1FFE,  "NIT, PMT or Elementary PID, etc." },
     {  0x1FFF, 0x1FFF,  "Null-packet" },
     {  0,0, NULL }
  };


  return findTableID (Table, i);
}



/*
  -- Scrambling Control  Table  ISO 13818-1  2.4.3.2
*/

char *dvbstrTS_ScramblingCtrl_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "not scrambled" },
     {  0x01, 0x01,  "user defined (scrambled?)" },
     {  0x02, 0x02,  "user defined (scrambled?)" },
     {  0x03, 0x03,  "user defined (scrambled?)" },
     {  0,0, NULL }
  };


  return findTableID (Table, i);
}


/*
  -- Adaption Field Type  ISO 13818-1  2.4.3.2
*/

char *dvbstrTS_AdaptionField_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "no adaption_field, payload only" },
     {  0x02, 0x02,  "adaption_field only, no payload" },
     {  0x03, 0x03,  "adaption_field followed by payload" },
     {  0,0, NULL }
  };


  return findTableID (Table, i);
}





/*
 ------------------------------------------------------------------------
  PES   Stuff
 ------------------------------------------------------------------------
*/


/*
  -- PES Stream_id  ISO 13818-1  2.4.3.6
*/

char *dvbstrPESstream_ID (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0xBB,  "???? report!" },
     {  0xBC, 0xBC,  "program_stream_map" },
     {  0xBD, 0xBD,  "private_stream_1" },
     {  0xBE, 0xBE,  "padding_stream" },
     {  0xBF, 0xBF,  "private_stream_2" },
     {  0xC0, 0xDF,  "ISO/IEC 13818-3 or ISO/IEC 11172-3 audio stream" },
     {  0xE0, 0xEF,  "ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 video stream" },
     {  0xF0, 0xF0,  "ECM_stream" },
     {  0xF1, 0xF1,  "EMM_stream" },
     {  0xF2, 0xF2,  "ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Annex-A or ISO/IEC 13818-6_DSMCC stream" },
     {  0xF3, 0xF3,  "ISO/IEC 13522 stream" },
     {  0xF4, 0xF4,  "ITU-T Rec. H.222.1 type A" },
     {  0xF5, 0xF5,  "ITU-T Rec. H.222.1 type B" },
     {  0xF6, 0xF6,  "ITU-T Rec. H.222.1 type C" },
     {  0xF7, 0xF7,  "ITU-T Rec. H.222.1 type D" },
     {  0xF8, 0xF8,  "ITU-T Rec. H.222.1 type E" },
     {  0xF9, 0xF9,  "ancillary_stream" },
     {  0xFA, 0xFE,  "reserved data stream" },
     {  0xFF, 0xFF,  "program_stream_directory" },
     {  0,0, NULL }
  };


  return findTableID (Table, i);
}


