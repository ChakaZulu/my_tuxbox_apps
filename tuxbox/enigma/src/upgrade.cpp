#include "upgrade.h"
#include <lib/gui/ebutton.h>
#include <unistd.h>
#include <xmltree.h>
#include <lib/gui/statusbar.h>
#include <lib/gui/emessage.h>
#include <lib/gui/eprogress.h>
#include <lib/gui/eskin.h>
#include <lib/dvb/decoder.h>
#include <lib/gdi/font.h>
#include <libmd5sum.h>
#include <lib/dvb/edvb.h>
#include <sys/mman.h>

#include <tuxbox.h>

#define TMP_IMAGE "/var/tmp/root.cramfs"
#define TMP_IMAGE_ALT "/var/tmp/cdk.cramfs"
#define TMP_CHANGELOG "/var/tmp/changelog"

static eString getVersionInfo(const char *info)
{
	FILE *f=fopen("/.version", "rt");
	if (!f)
		return "";
	eString result;
	while (1)
	{
		char buffer[128];
		if (!fgets(buffer, 128, f))
			break;
		if (strlen(buffer))
			buffer[strlen(buffer)-1]=0;
		if ((!strncmp(buffer, info, strlen(info)) && (buffer[strlen(info)]=='=')))
		{
			int i = strlen(info)+1;
			result = eString(buffer).mid(i, strlen(buffer)-i);
			break;
		}
	}	
	fclose(f);
	return result;
}

eListBoxEntryImage::eListBoxEntryImage
	(eListBox<eListBoxEntryImage> *listbox, eString name, eString target, eString url, eString version, eString creator, const unsigned char md5[16])
	: eListBoxEntryText((eListBox<eListBoxEntryText> *)listbox, name),
	name(name), target(target), url(url), version(version), creator(creator)
{
	if (md5)
		memcpy(this->md5, md5, 16);
	else
		memset(this->md5, 0, 16);
}

eHTTPDownload::eHTTPDownload(eHTTPConnection *c, const char *filename): eHTTPDataSource(c), filename(filename)
{
	if (c->remote_header.count("Content-Length"))
		total=atoi(c->remote_header["Content-Length"].c_str());
	else
		total=-1;
	received=0;
	fd=::creat(filename, 0777);
	progress(received, total);
}

eHTTPDownload::~eHTTPDownload()
{
	if (fd >= 0)
		::close(fd);
	if ((total != -1) && (total != received))
		::unlink(filename.c_str());
}

void eHTTPDownload::haveData(void *data, int len)
{
	if (len)
	{
		if (fd >= 0)
			::write(fd, data, len);
	}
	received+=len;
	progress(received, total);
}

eHTTPDownloadXML::eHTTPDownloadXML(eHTTPConnection *c, XMLTreeParser &parser): eHTTPDataSource(c), parser(parser)
{
	error=0;
	errorstring="";
}

void eHTTPDownloadXML::haveData(void *data, int len)
{
	if ((!error) && (!parser.Parse((char*)data, len, !data)))
	{
		errorstring.sprintf("XML parse error: %s at line %d",
			parser.ErrorString(parser.GetErrorCode()),
			parser.GetCurrentLineNumber());
		error=1;
	}
}

eUpgrade::eUpgrade()
:http(0), changelog(0)
{
	status = new eStatusBar(this);
	status->setFlags(eStatusBar::flagOwnerDraw);
	status->loadDeco();
	status->setName("status");

	images=new eListBox<eListBoxEntryImage>(this);
	images->setName("images");
	CONNECT(images->selected, eUpgrade::imageSelected);
	CONNECT(images->selchanged, eUpgrade::imageSelchanged);
	
	imagehelp=new eLabel(this);
	imagehelp->setName("imagehelp");
	imagehelp->setText(_("Please select the software version to upgrade to:"));

	progress=new eProgress(this);
	progress->setName("progress");
	progress->hide();
	
	progresstext=new eLabel(this);
	progresstext->setName("progresstext");
	progresstext->hide();
	
	changes=new eLabel(this, RS_WRAP);
	changes->setName("changes");
	
	abort=new eButton(this);
	abort->setName("abort");
	CONNECT(abort->selected, eUpgrade::abortDownload);
	abort->hide();

	if (eSkin::getActive()->build(this, "eUpgrade"))
		eFatal("skin load of \"eUpgrade\" failed");

	catalog=0;
	changelog=0;
	
	eString caturl=getVersionInfo("catalog");
	if (caturl.length())
		loadCatalog(caturl.c_str());
		
	ourversion=getVersionInfo("version");
	
	struct stat s;
	if (!stat(TMP_IMAGE_ALT, &s))
		rename(TMP_IMAGE_ALT, TMP_IMAGE);
	if (!stat(TMP_IMAGE, &s))
		new eListBoxEntryImage(images, _("manual upload"), "", "", "", "", 0);
}

void eUpgrade::loadCatalog(const char *url)
{
	current_url=url;
	int error;
	if (catalog)
		delete catalog;
	catalog=new XMLTreeParser("ISO-8859-1");
	http=eHTTPConnection::doRequest(url, eApp, &error);
	if (!http)
	{
		catalogTransferDone(error);
	} else
	{
		setStatus(_("downloading catalog..."));
		CONNECT(http->transferDone, eUpgrade::catalogTransferDone);
		CONNECT(http->createDataSource, eUpgrade::createCatalogDataSink);
		http->local_header["User-Agent"]="enigma-upgrade/1.0.0";
		http->start();
	}
}

void eUpgrade::loadChangelog(const char *url)
{
	current_url=url;
	int error;
	if (changelog)
		delete changelog;
	changelog=eHTTPConnection::doRequest(url, eApp, &error);
	if (!changelog)
	{
		changelogTransferDone(error);
	} else
	{
		setStatus(_("downloading changelog..."));
		CONNECT(changelog->transferDone, eUpgrade::changelogTransferDone);
		CONNECT(changelog->createDataSource, eUpgrade::createChangelogDataSink);
		changelog->local_header["User-Agent"]="enigma-upgrade/1.0.0";
		changelog->start();
	}
}

void eUpgrade::loadImage(const char *url)
{
	images->hide();
	imagehelp->hide();
	current_url=url;
	int error;
	if (http)
		delete http;
	progress->show();
	progresstext->show();
	abort->show();
	http=eHTTPConnection::doRequest(url, eApp, &error);
	if (!http)
	{
		imageTransferDone(error);
	} else
	{
		setStatus(_("downloading image..."));
		CONNECT(http->transferDone, eUpgrade::imageTransferDone);
		CONNECT(http->createDataSource, eUpgrade::createImageDataSink);
		http->local_header["User-Agent"]="enigma-upgrade/1.0.0";
		http->start();
	}
}

void eUpgrade::catalogTransferDone(int err)
{
	if ((!err) && http && (http->code == 200) && datacatalog && !datacatalog->error)
	{
		XMLTreeNode *root=catalog->RootNode();
		// FIXME
		eString mytarget("");
		// eString mytarget=eDVB::getInstance()->getInfo("mID").right(1);

		images->beginAtomic();
		for (XMLTreeNode *r=root->GetChild(); r; r=r->GetNext())
		{
			if (!strcmp(r->GetType(), "image"))
			{
				const char *name=r->GetAttributeValue("name");
				const char *url=r->GetAttributeValue("url");
				const char *version=r->GetAttributeValue("version");
				const char *target=r->GetAttributeValue("target");
				const char *creator=r->GetAttributeValue("creator");
				const char *amd5=r->GetAttributeValue("md5");
				unsigned char md5[16];
				if (!creator)
					creator=_("unknown");
				if (!amd5)
					continue;
				for (int i=0; i<32; i+=2)
				{
					char x[3];
					x[0]=amd5[i];
					if (!x[0])
						break;
					x[1]=amd5[i+1];
					if (!x[1])
						break;
					int v=0;
					if (sscanf(x, "%02x", &v) != 1)
						break;
					md5[i/2]=v;
				}
				if (!(name && url && version && target))
					continue;
				if (!strstr(target, mytarget.c_str()))
					continue;
				new eListBoxEntryImage(images, name, target, url, version, creator, md5);
			} else if (!strcmp(r->GetType(), "changelog"))
			{
				const char *changelog=r->GetAttributeValue("url");
				if (changelog)
					loadChangelog(changelog);
			}
		}
		setFocus(images);
		images->endAtomic();
		setStatus(_("Please select version to upgrade or LAME! to abort"));
		if (images->getCurrent())
			imageSelchanged(images->getCurrent());
	} else
	{
		if (err || http->code !=200)
			setError(err);
		else if (datacatalog)
		{
			eDebug("data error.");
			// setStatus(datacatalog->errorstring);
			setStatus("XML parse error.");
		}
	}
	if (catalog)
		delete catalog;
	http=0;
}

void eUpgrade::imageTransferDone(int err)
{
	progress->hide();
	progresstext->hide();
	abort->hide();
	if (err || !http || http->code != 200)
		setError(err);
	else
		flashImage(1);
	http=0;
	images->show();
	imagehelp->show();
}

void eUpgrade::changelogTransferDone(int err)
{
	if (err || !changelog || changelog->code != 200)
	{
		setError(err);
	} else
	{
		FILE *f=fopen(TMP_CHANGELOG, "rt");
		if (f)
		{
			char temp[1024];
			while (fgets(temp, 1024, f))
			{
				if (*temp)	
					temp[strlen(temp)-1]=0; // remove trailng \n
				eString str(temp);
				changelogEntry entry;
				
				entry.date=str.left(12);
				entry.priority=str[13]-'0';
				entry.text=str.mid(15);
				unsigned int in=entry.text.find(' ');
				entry.machines="*";
				if (in != eString::npos)
				{
					entry.machines=entry.text.left(in);
					entry.text=entry.text.mid(in+1);
				}
				changelogentries.push_back(entry);
			}
			fclose(f);
		}
		// FIXME
		displayChangelog(ourversion.mid(4), selectedversion.mid(4), "");
		// displayChangelog(ourversion.mid(4), selectedversion.mid(4), eDVB::getInstance()->getInfo("mID").right(1));
	}
	changelog=0;
}

void eUpgrade::imageSelected(eListBoxEntryImage *img)
{
	if (img)
	{
		if (img->url.length())
		{
			memcpy(expected_md5, img->md5, 16);
			setStatus(img->url);
			loadImage(img->url.c_str());
		} else
			flashImage(0);
	} else
		close(0); // aborted
}

void eUpgrade::imageSelchanged(eListBoxEntryImage *img)
{
	selectedversion=img->version;
	// FIXME
	displayChangelog(ourversion.mid(4), selectedversion.mid(4), "");
	// displayChangelog(ourversion.mid(4), selectedversion.mid(4), eDVB::getInstance()->getInfo("mID").right(1));
}

void eUpgrade::setStatus(const eString &string)
{
	status->setText(string);
}

void eUpgrade::setError(int err)
{
	eString errmsg;
	switch (err)
	{
	case 0:
		if (http && http->code != 200)
			errmsg="error: server replied " + eString().setNum(http->code) + " " + http->code_descr;
		break;
	case -2:
		errmsg="Can't resolve hostname!";
		break;
	case -3:
		errmsg="Can't connect! (check network settings)";
		break;
	default:
		errmsg.sprintf("unknown error %d", err);
	}
	setStatus(errmsg);
	if (errmsg.length())
	{
		if (current_url.length())
			errmsg+="\n(URL: " + current_url + ")";
		eMessageBox box(errmsg, _("Error!"), eMessageBox::btOK|eMessageBox::iconError);
		box.show();
		box.exec();
		box.hide();
	}
}

eHTTPDataSource *eUpgrade::createCatalogDataSink(eHTTPConnection *conn)
{
	return datacatalog=new eHTTPDownloadXML(conn, *catalog);
}

eHTTPDataSource *eUpgrade::createImageDataSink(eHTTPConnection *conn)
{
	image=new eHTTPDownload(conn, TMP_IMAGE);
	lasttime=0;
	CONNECT(image->progress, eUpgrade::downloadProgress);
	return image;
}

eHTTPDataSource *eUpgrade::createChangelogDataSink(eHTTPConnection *conn)
{
	changelogdownload=new eHTTPDownload(conn, TMP_CHANGELOG);
	lasttime=0;
	CONNECT(changelogdownload->progress, eUpgrade::downloadProgress);
	return changelogdownload;
}

void eUpgrade::downloadProgress(int received, int total)
{
	if ((time(0) == lasttime) && (received != total))
		return;
	lasttime=time(0);
	if (total > 0)
	{
		eString pt;
		int perc=received*100/total;
		pt.sprintf("%d/%d kb (%d%%)", received/1024, total/1024, perc);
		progress->setPerc(perc);
		progresstext->setText(pt);
	} else
	{
		eString pt;
		pt.sprintf("%d kb", received/1024);
		progress->setPerc(0);
		progresstext->setText(pt);
	}
}

void eUpgrade::abortDownload()
{
	if (http)
	{
		delete http;
		http=0;
	}
	setStatus(_("Download aborted."));
	progress->hide();
	progresstext->hide();
	abort->hide();
	images->show();
	imagehelp->show();
}

void eUpgrade::flashImage(int checkmd5)
{
	setStatus(_("checking consistency of file..."));
	unsigned char md5[16];
	if (checkmd5 && md5_file (TMP_IMAGE, 1, (unsigned char*) &md5))
	{
		setStatus(_("write error while downloading..."));
		eMessageBox mb(
			_("write error while downloading..."),
			_("Error!"),
			eMessageBox::btOK|eMessageBox::iconError);
		hide();
		mb.show();
		mb.exec();
		mb.hide();
		show();
	} else
	{
		if (checkmd5 && memcmp(md5, expected_md5, 16))
		{
			setStatus(_("Data error. The checksum didn't match."));
			eMessageBox mb(
				_("Data error. The checksum didn't match."),
				_("Error!"),
				eMessageBox::btOK|eMessageBox::iconError);
			hide();
			mb.show();
			mb.exec();
			mb.hide();
			show();
		} else
		{
			setStatus(_("Checksum OK. Ready to upgrade."));
			eMessageBox mb(
				_("Are you sure you want to upgrade to this new version?"),
				_("Ready to upgrade"),
				eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion);
			int mtdsize;
			hide();
			mb.show();
			int res=mb.exec();
			mb.hide();
			if (res == eMessageBox::btYes)
			{
				eMessageBox mb(
					_("Erasing...\nPlease wait... do NOT switch off the receiver!"),
					_("upgrade in progress"), eMessageBox::iconInfo);
				mb.show();
				sync();
				Decoder::Flush();
				eString mtd;

				switch (tuxbox_get_model())
				{
				case TUXBOX_MODEL_DBOX2:
					mtd="2";
					mtdsize=0x6e0000;
					break;
				case TUXBOX_MODEL_DREAMBOX:
					mtd="0";
					mtdsize=0x700000;
					break;
				default:
					mtd="../null";
					mtdsize=0;
				}

				{
					int fd=open(eString("/dev/mtdblock/" + mtd).c_str(), O_RDONLY);
					void *ptr;
					volatile int a;
					if ((fd < 0) || !(ptr=mmap(0, mtdsize, PROT_READ, MAP_SHARED|MAP_LOCKED, fd, 0)))

					{
						eMessageBox mb(
							_("upgrade failed with errorcode UD0"),
							_("upgrade failed"),
							eMessageBox::btOK|eMessageBox::iconError);
						mb.show();
						mb.exec();
						mb.hide();
						return;
					}
					for (int i=0; i<mtdsize; i+=4096) // page size
						a=((__u8*)ptr)[i];
				}
				int res=system(eString("/bin/eraseall /dev/mtd/" + mtd).c_str())>>8;
				mb.hide();
				if (!res)
				{
					eMessageBox mb(
						_("Writing software to flash...\nPlease wait... do NOT switch off the receiver!"),
						_("upgrade in progress"), eMessageBox::iconInfo);
					mb.show();
					res=system(eString("cat " TMP_IMAGE " > /dev/mtd/" + mtd).c_str())>>8;
					mb.hide();
					if (!res)
					{
						eMessageBox mb(
							_("upgrade successful!\nrestarting..."),
							_("upgrade ok"),
						eMessageBox::btOK|eMessageBox::iconInfo);
						mb.show();
						mb.exec();
						mb.hide();
						system("/sbin/reboot");
						system("/bin/reboot");
						exit(0);
					}
				}
				if (res)
				{
					eMessageBox mb(
						_("upgrade failed with errorcode UA15"),
						_("upgrade failed"),
						eMessageBox::btOK|eMessageBox::iconError);
					mb.show();
					mb.exec();
					mb.hide();
				}
			} else
				close(0);
		}
	}
}

void eUpgrade::displayChangelog(eString oldversion, eString newversion, eString mid)
{
	eString changetext;
	if (newversion.empty())
	{
		changetext="";
	} else if (oldversion == newversion)
	{
		changetext=_("This is the currently installed release.");
	} else if (oldversion > newversion)
	{
		changetext=_("This is an older release.");
	} else if (changelogentries.empty())
	{
		changetext=_("No changelog data available.");
	} else
	{
		// build list of changetext
		
		std::multimap<int,eString> pchanges;
		for (std::list<changelogEntry>::const_iterator i(changelogentries.begin());
				i != changelogentries.end(); ++i)
		{
					// check if change is new for this version
			if ((i->date > oldversion) && (i->date <= newversion) && ((i->machines=="*") || (i->machines.find(mid) != eString::npos)))
				pchanges.insert(std::pair<int,eString>(i->priority, i->text));
		}
		if (pchanges.empty())
			changetext=_("No new features were added in this release.");
		else
		{
			changetext=_("The following new features were added in this release:\n");
			for (std::map<int,eString>::const_iterator i(pchanges.begin());
					i != pchanges.end(); ++i)
				changetext+=" * " + i->second + "\n";
		}
	}
	changes->setText(changetext);
}

eUpgrade::~eUpgrade()
{
	if (http)
		delete http;
	if (changelog)
		delete changelog;
}
