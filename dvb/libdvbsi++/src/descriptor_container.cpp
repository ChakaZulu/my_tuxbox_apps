/*
 * $Id: descriptor_container.cpp,v 1.4 2005/09/30 16:13:49 ghostrider Exp $
 *
 * Copyright (C) 2002-2004 Andreas Oberritter <obi@saftware.de>
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

#include <dvbsi++/ac3_descriptor.h>
#include <dvbsi++/ancillary_data_descriptor.h>
#include <dvbsi++/announcement_support_descriptor.h>
#include <dvbsi++/application_descriptor.h>
#include <dvbsi++/application_icons_descriptor.h>
#include <dvbsi++/application_name_descriptor.h>
#include <dvbsi++/application_signalling_descriptor.h>
#include <dvbsi++/application_storage_descriptor.h>
#include <dvbsi++/audio_stream_descriptor.h>
#include <dvbsi++/bouquet_name_descriptor.h>
#include <dvbsi++/ca_descriptor.h>
#include <dvbsi++/ca_identifier_descriptor.h>
#include <dvbsi++/cable_delivery_system_descriptor.h>
#include <dvbsi++/caching_priority_descriptor.h>
#include <dvbsi++/carousel_identifier_descriptor.h>
#include <dvbsi++/cell_frequency_link_descriptor.h>
#include <dvbsi++/cell_list_descriptor.h>
#include <dvbsi++/crc32_descriptor.h>
#include <dvbsi++/component_descriptor.h>
#include <dvbsi++/compressed_module_descriptor.h>
#include <dvbsi++/content_descriptor.h>
#include <dvbsi++/content_type_descriptor.h>
#include <dvbsi++/country_availability_descriptor.h>
#include <dvbsi++/data_broadcast_descriptor.h>
#include <dvbsi++/data_broadcast_id_descriptor.h>
#include <dvbsi++/delegated_application_descriptor.h>
#include <dvbsi++/descriptor_container.h>
#include <dvbsi++/descriptor_tag.h>
#include <dvbsi++/dii_location_descriptor.h>
#include <dvbsi++/dvb_html_application_boundary_descriptor.h>
#include <dvbsi++/dvb_html_application_descriptor.h>
#include <dvbsi++/dvb_html_application_location_descriptor.h>
#include <dvbsi++/dvb_j_application_descriptor.h>
#include <dvbsi++/dvb_j_application_location_descriptor.h>
#include <dvbsi++/est_download_time_descriptor.h>
#include <dvbsi++/extended_event_descriptor.h>
#include <dvbsi++/external_application_authorisation_descriptor.h>
#include <dvbsi++/frequency_list_descriptor.h>
#include <dvbsi++/group_link_descriptor.h>
#include <dvbsi++/info_descriptor.h>
#include <dvbsi++/ip_signaling_descriptor.h>
#include <dvbsi++/iso639_language_descriptor.h>
#include <dvbsi++/label_descriptor.h>
#include <dvbsi++/linkage_descriptor.h>
#include <dvbsi++/local_time_offset_descriptor.h>
#include <dvbsi++/location_descriptor.h>
#include <dvbsi++/module_link_descriptor.h>
#include <dvbsi++/mosaic_descriptor.h>
#include <dvbsi++/multilingual_bouquet_name_descriptor.h>
#include <dvbsi++/multilingual_component_descriptor.h>
#include <dvbsi++/multilingual_network_name_descriptor.h>
#include <dvbsi++/multilingual_service_name_descriptor.h>
#include <dvbsi++/name_descriptor.h>
#include <dvbsi++/network_name_descriptor.h>
#include <dvbsi++/nvod_reference_descriptor.h>
#include <dvbsi++/parental_rating_descriptor.h>
#include <dvbsi++/pdc_descriptor.h>
#include <dvbsi++/plugin_descriptor.h>
#include <dvbsi++/prefetch_descriptor.h>
#include <dvbsi++/private_data_specifier_descriptor.h>
#include <dvbsi++/satellite_delivery_system_descriptor.h>
#include <dvbsi++/service_descriptor.h>
#include <dvbsi++/service_identifier_descriptor.h>
#include <dvbsi++/service_list_descriptor.h>
#include <dvbsi++/service_move_descriptor.h>
#include <dvbsi++/short_event_descriptor.h>
#include <dvbsi++/stream_identifier_descriptor.h>
#include <dvbsi++/stuffing_descriptor.h>
#include <dvbsi++/subtitling_descriptor.h>
#include <dvbsi++/target_background_grid_descriptor.h>
#include <dvbsi++/telephone_descriptor.h>
#include <dvbsi++/teletext_descriptor.h>
#include <dvbsi++/terrestrial_delivery_system_descriptor.h>
#include <dvbsi++/time_shifted_service_descriptor.h>
#include <dvbsi++/transport_protocol_descriptor.h>
#include <dvbsi++/type_descriptor.h>
#include <dvbsi++/unknown_descriptor.h>
#include <dvbsi++/vbi_data_descriptor.h>
#include <dvbsi++/vbi_teletext_descriptor.h>
#include <dvbsi++/video_stream_descriptor.h>
#include <dvbsi++/video_window_descriptor.h>

DescriptorContainer::~DescriptorContainer(void)
{
	for (DescriptorIterator i = descriptorList.begin(); i != descriptorList.end(); ++i)
		delete *i;
}

void DescriptorContainer::descriptor(const uint8_t * const buffer, const enum DescriptorScope scope, bool back)
{
	void (DescriptorList::*pushFunc) (Descriptor* const &) =
		back ? & DescriptorList::push_back
			: & DescriptorList::push_front;

	switch (scope) {
	case SCOPE_SI:
		descriptorSi(buffer, back);
		break;
	case SCOPE_CAROUSEL:
		descriptorCarousel(buffer, back);
		break;
	case SCOPE_MHP:
		descriptorMhp(buffer, back);
		break;
	default:
		(descriptorList.*pushFunc)(new Descriptor(buffer));
		break;
	}
}

void DescriptorContainer::descriptorSi(const uint8_t * const buffer, bool back)
{
	void (DescriptorList::*pushFunc) (Descriptor* const &) =
		back ? & DescriptorList::push_back
			: & DescriptorList::push_front;

	switch (buffer[0]) {
	case VIDEO_STREAM_DESCRIPTOR:
		(descriptorList.*pushFunc)(new VideoStreamDescriptor(buffer));
		break;

	case AUDIO_STREAM_DESCRIPTOR:
		(descriptorList.*pushFunc)(new AudioStreamDescriptor(buffer));
		break;

	case TARGET_BACKGROUND_GRID_DESCRIPTOR:
		(descriptorList.*pushFunc)(new TargetBackgroundGridDescriptor(buffer));
		break;

	case VIDEO_WINDOW_DESCRIPTOR:
		(descriptorList.*pushFunc)(new VideoWindowDescriptor(buffer));
		break;

	case CA_DESCRIPTOR:
		(descriptorList.*pushFunc)(new CaDescriptor(buffer));
		break;

	case ISO_639_LANGUAGE_DESCRIPTOR:
		(descriptorList.*pushFunc)(new Iso639LanguageDescriptor(buffer));
		break;

	case CAROUSEL_IDENTIFIER_DESCRIPTOR:
		(descriptorList.*pushFunc)(new CarouselIdentifierDescriptor(buffer));
		break;

	case NETWORK_NAME_DESCRIPTOR:
		(descriptorList.*pushFunc)(new NetworkNameDescriptor(buffer));
		break;

	case SERVICE_LIST_DESCRIPTOR:
		(descriptorList.*pushFunc)(new ServiceListDescriptor(buffer));
		break;

	case STUFFING_DESCRIPTOR:
		(descriptorList.*pushFunc)(new StuffingDescriptor(buffer));
		break;

	case SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR:
		(descriptorList.*pushFunc)(new SatelliteDeliverySystemDescriptor(buffer));
		break;

	case CABLE_DELIVERY_SYSTEM_DESCRIPTOR:
		(descriptorList.*pushFunc)(new CableDeliverySystemDescriptor(buffer));
		break;

	case VBI_DATA_DESCRIPTOR:
		(descriptorList.*pushFunc)(new VbiDataDescriptor(buffer));
		break;

	case VBI_TELETEXT_DESCRIPTOR:
		(descriptorList.*pushFunc)(new VbiTeletextDescriptor(buffer));
		break;

	case BOUQUET_NAME_DESCRIPTOR:
		(descriptorList.*pushFunc)(new BouquetNameDescriptor(buffer));
		break;

	case SERVICE_DESCRIPTOR:
		(descriptorList.*pushFunc)(new ServiceDescriptor(buffer));
		break;

	case COUNTRY_AVAILABILITY_DESCRIPTOR:
		(descriptorList.*pushFunc)(new CountryAvailabilityDescriptor(buffer));
		break;

	case LINKAGE_DESCRIPTOR:
		(descriptorList.*pushFunc)(new LinkageDescriptor(buffer));
		break;

	case NVOD_REFERENCE_DESCRIPTOR:
		(descriptorList.*pushFunc)(new NvodReferenceDescriptor(buffer));
		break;

	case TIME_SHIFTED_SERVICE_DESCRIPTOR:
		(descriptorList.*pushFunc)(new TimeShiftedServiceDescriptor(buffer));
		break;

	case SHORT_EVENT_DESCRIPTOR:
		(descriptorList.*pushFunc)(new ShortEventDescriptor(buffer));
		break;

	case EXTENDED_EVENT_DESCRIPTOR:
		(descriptorList.*pushFunc)(new ExtendedEventDescriptor(buffer));
		break;

	case COMPONENT_DESCRIPTOR:
		(descriptorList.*pushFunc)(new ComponentDescriptor(buffer));
		break;

	case MOSAIC_DESCRIPTOR:
		(descriptorList.*pushFunc)(new MosaicDescriptor(buffer));
		break;

	case STREAM_IDENTIFIER_DESCRIPTOR:
		(descriptorList.*pushFunc)(new StreamIdentifierDescriptor(buffer));
		break;

	case CA_IDENTIFIER_DESCRIPTOR:
		(descriptorList.*pushFunc)(new CaIdentifierDescriptor(buffer));
		break;

	case CONTENT_DESCRIPTOR:
		(descriptorList.*pushFunc)(new ContentDescriptor(buffer));
		break;

	case PARENTAL_RATING_DESCRIPTOR:
		(descriptorList.*pushFunc)(new ParentalRatingDescriptor(buffer));
		break;

	case TELETEXT_DESCRIPTOR:
		(descriptorList.*pushFunc)(new TeletextDescriptor(buffer));
		break;

	case TELEPHONE_DESCRIPTOR:
		(descriptorList.*pushFunc)(new TelephoneDescriptor(buffer));
		break;

	case LOCAL_TIME_OFFSET_DESCRIPTOR:
		(descriptorList.*pushFunc)(new LocalTimeOffsetDescriptor(buffer));
		break;

	case SUBTITLING_DESCRIPTOR:
		(descriptorList.*pushFunc)(new SubtitlingDescriptor(buffer));
		break;

	case TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR:
		(descriptorList.*pushFunc)(new TerrestrialDeliverySystemDescriptor(buffer));
		break;

	case MULTILINGUAL_NETWORK_NAME_DESCRIPTOR:
		(descriptorList.*pushFunc)(new MultilingualNetworkNameDescriptor(buffer));
		break;

	case MULTILINGUAL_BOUQUET_NAME_DESCRIPTOR:
		(descriptorList.*pushFunc)(new MultilingualBouquetNameDescriptor(buffer));
		break;

	case MULTILINGUAL_SERVICE_NAME_DESCRIPTOR:
		(descriptorList.*pushFunc)(new MultilingualServiceNameDescriptor(buffer));
		break;

	case MULTILINGUAL_COMPONENT_DESCRIPTOR:
		(descriptorList.*pushFunc)(new MultilingualComponentDescriptor(buffer));
		break;

	case PRIVATE_DATA_SPECIFIER_DESCRIPTOR:
		(descriptorList.*pushFunc)(new PrivateDataSpecifierDescriptor(buffer));
		break;

	case SERVICE_MOVE_DESCRIPTOR:
		(descriptorList.*pushFunc)(new ServiceMoveDescriptor(buffer));
		break;

	case FREQUENCY_LIST_DESCRIPTOR:
		(descriptorList.*pushFunc)(new FrequencyListDescriptor(buffer));
		break;

	case DATA_BROADCAST_DESCRIPTOR:
		(descriptorList.*pushFunc)(new DataBroadcastDescriptor(buffer));
		break;

	case DATA_BROADCAST_ID_DESCRIPTOR:
		(descriptorList.*pushFunc)(new DataBroadcastIdDescriptor(buffer));
		break;

	case PDC_DESCRIPTOR:
		(descriptorList.*pushFunc)(new PdcDescriptor(buffer));
		break;

	case AC3_DESCRIPTOR:
		(descriptorList.*pushFunc)(new Ac3Descriptor(buffer));
		break;

	case ANCILLARY_DATA_DESCRIPTOR:
		(descriptorList.*pushFunc)(new AncillaryDataDescriptor(buffer));
		break;

	case CELL_LIST_DESCRIPTOR:
		(descriptorList.*pushFunc)(new CellListDescriptor(buffer));
		break;

	case CELL_FREQUENCY_LINK_DESCRIPTOR:
		(descriptorList.*pushFunc)(new CellFrequencyLinkDescriptor(buffer));
		break;

	case ANNOUNCEMENT_SUPPORT_DESCRIPTOR:
		(descriptorList.*pushFunc)(new AnnouncementSupportDescriptor(buffer));
		break;

	case APPLICATION_SIGNALLING_DESCRIPTOR:
		(descriptorList.*pushFunc)(new ApplicationSignallingDescriptor(buffer));
		break;

	case SERVICE_IDENTIFIER_DESCRIPTOR:
		(descriptorList.*pushFunc)(new ServiceIdentifierDescriptor(buffer));
		break;

	default:
		(descriptorList.*pushFunc)(new UnknownDescriptor(buffer));
		break;
	}
}

void DescriptorContainer::descriptorCarousel(const uint8_t * const buffer, bool back)
{
	void (DescriptorList::*pushFunc) (Descriptor* const &) =
		back ? & DescriptorList::push_back
			: & DescriptorList::push_front;

	switch (buffer[0]) {
	case TYPE_DESCRIPTOR:
		(descriptorList.*pushFunc)(new TypeDescriptor(buffer));
		break;

	case NAME_DESCRIPTOR:
		(descriptorList.*pushFunc)(new NameDescriptor(buffer));
		break;

	case INFO_DESCRIPTOR:
		(descriptorList.*pushFunc)(new InfoDescriptor(buffer));
		break;

	case MODULE_LINK_DESCRIPTOR:
		(descriptorList.*pushFunc)(new ModuleLinkDescriptor(buffer));
		break;

	case CRC32_DESCRIPTOR:
		(descriptorList.*pushFunc)(new Crc32Descriptor(buffer));
		break;

	case LOCATION_DESCRIPTOR:
		(descriptorList.*pushFunc)(new LocationDescriptor(buffer));
		break;

	case EST_DOWNLOAD_TIME_DESCRIPTOR:
		(descriptorList.*pushFunc)(new EstDownloadTimeDescriptor(buffer));
		break;

	case GROUP_LINK_DESCRIPTOR:
		(descriptorList.*pushFunc)(new GroupLinkDescriptor(buffer));
		break;

	case COMPRESSED_MODULE_DESCRIPTOR:
		(descriptorList.*pushFunc)(new CompressedModuleDescriptor(buffer));
		break;

	case LABEL_DESCRIPTOR:
		(descriptorList.*pushFunc)(new LabelDescriptor(buffer));
		break;

	case CACHING_PRIORITY_DESCRIPTOR:
		(descriptorList.*pushFunc)(new CachingPriorityDescriptor(buffer));
		break;

	case CONTENT_TYPE_DESCRIPTOR:
		(descriptorList.*pushFunc)(new ContentTypeDescriptor(buffer));
		break;

	case PRIVATE_DATA_SPECIFIER_DESCRIPTOR:
		(descriptorList.*pushFunc)(new PrivateDataSpecifierDescriptor(buffer));
		break;

	default:
		(descriptorList.*pushFunc)(new Descriptor(buffer));
		break;
	}
}

void DescriptorContainer::descriptorMhp(const uint8_t * const buffer, bool back)
{
	void (DescriptorList::*pushFunc) (Descriptor* const &) =
		back ? & DescriptorList::push_back
			: & DescriptorList::push_front;

	switch (buffer[0]) {
	case APPLICATION_DESCRIPTOR:
		(descriptorList.*pushFunc)(new ApplicationDescriptor(buffer));
		break;

	case APPLICATION_NAME_DESCRIPTOR:
		(descriptorList.*pushFunc)(new ApplicationNameDescriptor(buffer));
		break;

	case TRANSPORT_PROTOCOL_DESCRIPTOR:
		(descriptorList.*pushFunc)(new TransportProtocolDescriptor(buffer));
		break;

	case DVB_J_APPLICATION_DESCRIPTOR:
		(descriptorList.*pushFunc)(new DvbJApplicationDescriptor(buffer));
		break;

	case DVB_J_APPLICATION_LOCATION_DESCRIPTOR:
		(descriptorList.*pushFunc)(new DvbJApplicationLocationDescriptor(buffer));
		break;

	case EXTERNAL_APPLICATION_AUTHORISATION_DESCRIPTOR:
		(descriptorList.*pushFunc)(new ExternalApplicationAuthorisationDescriptor(buffer));
		break;

	case DVB_HTML_APPLICATION_DESCRIPTOR:
		(descriptorList.*pushFunc)(new DvbHtmlApplicationDescriptor(buffer));
		break;

	case DVB_HTML_APPLICATION_LOCATION_DESCRIPTOR:
		(descriptorList.*pushFunc)(new DvbHtmlApplicationLocationDescriptor(buffer));
		break;

	case DVB_HTML_APPLICATION_BOUNDARY_DESCRIPTOR:
		(descriptorList.*pushFunc)(new DvbHtmlApplicationBoundaryDescriptor(buffer));
		break;

	case APPLICATION_ICONS_DESCRIPTOR:
		(descriptorList.*pushFunc)(new ApplicationIconsDescriptor(buffer));
		break;

	case PREFETCH_DESCRIPTOR:
		(descriptorList.*pushFunc)(new PrefetchDescriptor(buffer));
		break;

	case DII_LOCATION_DESCRIPTOR:
		(descriptorList.*pushFunc)(new DiiLocationDescriptor(buffer));
		break;

	case DELEGATED_APPLICATION_DESCRIPTOR:
		(descriptorList.*pushFunc)(new DelegatedApplicationDescriptor(buffer));
		break;

	case PLUGIN_DESCRIPTOR:
		(descriptorList.*pushFunc)(new PluginDescriptor(buffer));
		break;

	case APPLICATION_STORAGE_DESCRIPTOR:
		(descriptorList.*pushFunc)(new ApplicationStorageDescriptor(buffer));
		break;

	case IP_SIGNALING_DESCRIPTOR:
		(descriptorList.*pushFunc)(new IpSignalingDescriptor(buffer));
		break;

	case PRIVATE_DATA_SPECIFIER_DESCRIPTOR:
		(descriptorList.*pushFunc)(new PrivateDataSpecifierDescriptor(buffer));
		break;

	default:
		(descriptorList.*pushFunc)(new Descriptor(buffer));
		break;
	}
}

const DescriptorList *DescriptorContainer::getDescriptors(void) const
{
	return &descriptorList;
}
