//
// $Id: infoviewer.cpp,v 1.14 2001/09/13 17:15:51 field Exp $
//
// $Log: infoviewer.cpp,v $
// Revision 1.14  2001/09/13 17:15:51  field
// verbessertes warten aufs epg
//
// Revision 1.13  2001/09/13 10:12:41  field
// Major update! Beschleunigtes zappen & EPG uvm...
//
// Revision 1.12  2001/09/09 23:53:46  fnbrd
// Fixed some bugs, only shown compiling with -Os.
// Conclusion: use -Os ;)
//
// Revision 1.11  2001/09/05 21:21:16  McClean
// design-fix
//
// Revision 1.10  2001/08/22 11:29:31  McClean
// infoviewer designfix
//
// Revision 1.9  2001/08/20 13:07:10  tw-74
// cosmetic changes and changes for variable font size
//
// Revision 1.8  2001/08/16 23:19:18  McClean
// epg-view and quickview changed
//
// Revision 1.7  2001/08/16 00:19:44  fnbrd
// Removed debug output.
//
//

#include "infoviewer.h"

CInfoViewer::CInfoViewer()
{
	intShowDuration = 15; //7,5 sec
	frameBuffer = NULL;
	fonts = NULL;
    BoxStartX=BoxStartY=BoxEndX=BoxEndY=0;

	strcpy( running, "");
	strcpy( next, "");
	strcpy( runningStart, "");
	strcpy( nextStart, "");
	strcpy( runningDuration, "");
	strcpy( nextDuration, "");
	runningPercent = 0;
	CurrentChannel = "";

    pthread_cond_init( &epg_cond, NULL );
    pthread_mutex_init( &epg_mutex, NULL );

    if (pthread_create (&thrViewer, NULL, InfoViewerThread, (void *) this) != 0 )
	{
		perror("create failed\n");
	}
}

void CInfoViewer::start(CFrameBuffer *FrameBuffer, FontsDef *Fonts, SNeutrinoSettings *Settings, CRCInput *AInput )
{
	frameBuffer = FrameBuffer;
	fonts = Fonts;
	settings = Settings;
    rcInput = AInput;

	InfoHeightY = fonts->infobar_number->getHeight()*9/8 +
                  2*fonts->infobar_info->getHeight() +
//                  fonts->infobar_small->getHeight() +
                  25;

//	printf("infoh %d", InfoHeightY);

    ChanWidth = fonts->infobar_number->getRenderWidth("000") + 10;
	ChanHeight = fonts->infobar_number->getHeight()*9/8;

}

void CInfoViewer::setDuration( int Duration )
{
	intShowDuration = Duration;
}

void CInfoViewer::setStreamInfo( CStreamInfo *Info )
{
    StreamInfo = Info;
}

void CInfoViewer::showTitle( int ChanNum, string Channel, bool CalledFromNumZap )
{
    pthread_mutex_lock( &epg_mutex );
	CurrentChannel = Channel;
    if ( CalledFromNumZap )
    {
        EPG_NotFound_Text = "EPG noch nicht geladen.";
    }
    else
    {
        EPG_NotFound_Text =  "Warte auf EPG...";
    }
    pthread_mutex_unlock( &epg_mutex );

	BoxStartX = settings->screen_StartX+ 20;
	BoxEndX   = settings->screen_EndX- 20;
	BoxEndY   = settings->screen_EndY- 20;
	BoxStartY = BoxEndY- InfoHeightY;

	//frameBuffer->paintVLine(settings->screen_StartX,0,576, 3);
	//frameBuffer->paintVLine(settings->screen_EndX,0,576, 3);
	//frameBuffer->paintHLine(0,719, settings->screen_EndY,3);


	//number box
	frameBuffer->paintBoxRel(BoxStartX+10, BoxStartY+10, ChanWidth, ChanHeight, COL_INFOBAR_SHADOW);
	frameBuffer->paintBoxRel(BoxStartX,    BoxStartY,    ChanWidth, ChanHeight, COL_INFOBAR);

	//channel number
	char strChanNum[10];
	sprintf( (char*) strChanNum, "%d", ChanNum);
	int ChanNumXPos = BoxStartX + ((ChanWidth - fonts->infobar_number->getRenderWidth(strChanNum))>>1);
	fonts->infobar_number->RenderString(ChanNumXPos, BoxStartY+ChanHeight, ChanWidth, strChanNum, COL_INFOBAR);

	//infobox
	int ChanNameX = BoxStartX + ChanWidth + 10;
	int ChanNameY = BoxStartY + (ChanHeight>>1)   + 5; //oberkante schatten?
	frameBuffer->paintBox(ChanNameX, ChanNameY, BoxEndX, BoxEndY, COL_INFOBAR);
	
	// ... with channel name
	int height=fonts->infobar_channame->getHeight()+5;
	fonts->infobar_channame->RenderString(ChanNameX+15, ChanNameY+height, BoxEndX-ChanNameX-135, Channel.c_str(), COL_INFOBAR);

	int ChanInfoX = BoxStartX + (ChanWidth >>1);
	int ChanInfoY = BoxStartY + ChanHeight+10;
	frameBuffer->paintBox(ChanInfoX, ChanInfoY, ChanNameX, BoxEndY, COL_INFOBAR);

    is_visible = true;
    pthread_cond_signal( &epg_cond );

    usleep(50);

    int key;

    do
    {
        key = rcInput->getKey( intShowDuration* 5 );
        if ( key == CRCInput::RC_blue )
        {
            StreamInfo->exec(frameBuffer, rcInput, NULL, "");
            key = -1;
        }

    } while (false);

    if ( ( key != CRCInput::RC_timeout ) &&
         ( ( key != CRCInput::RC_ok ) || ( CalledFromNumZap ) ) &&
         ( ( key != CRCInput::RC_home ) || ( CalledFromNumZap ) ) &&
         ( key != -1 ) )
    {
        rcInput->addKey2Buffer(key);
    };

    if ( ( key != settings->key_quickzap_up ) &&
         ( key != settings->key_quickzap_down ) &&
         ( key != CRCInput::RC_help ) &&
         ( !CalledFromNumZap ) )
    {
        killTitle();
    };
}

void CInfoViewer::showData()
{
	int height;

	int ChanNameY = BoxStartY + (ChanHeight>>1)+3;

	int ChanInfoX = BoxStartX + (ChanWidth >>1);
	int ChanInfoY = BoxStartY + ChanHeight+ 15; //+10
	

	//percent
	height=fonts->infobar_channame->getHeight()/3;
	int height2= int( fonts->infobar_channame->getHeight()/1.5);
	frameBuffer->paintBoxRel(BoxEndX-114, ChanNameY+height,   2+100+2, height2, COL_INFOBAR+7);
	frameBuffer->paintBoxRel(BoxEndX-112, ChanNameY+height+2, runningPercent+2, height2-4, COL_INFOBAR+5);
	frameBuffer->paintBoxRel(BoxEndX-112+runningPercent, ChanNameY+height+2, 100-runningPercent, height2-4, COL_INFOBAR+2);

	//info running
	int start1width      = fonts->infobar_info->getRenderWidth(runningStart);
	int duration1Width   = fonts->infobar_info->getRenderWidth(runningDuration); 
	int duration1TextPos = BoxEndX-duration1Width-10;
	height=fonts->infobar_info->getHeight();
	fonts->infobar_info->RenderString(ChanInfoX+10,                ChanInfoY+height, start1width, runningStart, COL_INFOBAR);
	fonts->infobar_info->RenderString(ChanInfoX+10+start1width+10, ChanInfoY+height, duration1TextPos-(ChanInfoX+10+start1width+10)-10, running, COL_INFOBAR);
	fonts->infobar_info->RenderString(duration1TextPos,            ChanInfoY+height, duration1Width, runningDuration, COL_INFOBAR);

	ChanInfoY += height;

	//info next
    frameBuffer->paintBox(BoxStartX + ChanWidth + 25, ChanInfoY, BoxEndX, ChanInfoY+ height , COL_INFOBAR);

	int start2width      = fonts->infobar_info->getRenderWidth(nextStart);
	int duration2Width   = fonts->infobar_info->getRenderWidth(nextDuration); 
	int duration2TextPos = BoxEndX-duration2Width-10;
	fonts->infobar_info->RenderString(ChanInfoX+10,                ChanInfoY+height, start2width, nextStart, COL_INFOBAR);
	fonts->infobar_info->RenderString(ChanInfoX+10+start2width+10, ChanInfoY+height, duration2TextPos-(ChanInfoX+10+start2width+10)-10, next, COL_INFOBAR);
	fonts->infobar_info->RenderString(duration2TextPos,            ChanInfoY+height, duration2Width, nextDuration, COL_INFOBAR);
}

void CInfoViewer::showWarte()
{
	int height=fonts->infobar_info->getHeight();
    int ChanInfoY = BoxStartY + ChanHeight+ 15+ 2* height;
    int xStart= BoxStartX + ChanWidth + 25;

    pthread_mutex_trylock( &epg_mutex );
	fonts->infobar_info->RenderString(xStart, ChanInfoY, BoxEndX- xStart, EPG_NotFound_Text, COL_INFOBAR);
    pthread_mutex_unlock( &epg_mutex );
}

void CInfoViewer::killTitle()
{
	frameBuffer->paintBackgroundBox(BoxStartX, BoxStartY, BoxEndX, BoxEndY );
    is_visible = false;
}


void * CInfoViewer::InfoViewerThread (void *arg)
{
    int repCount;
    string query = "";
    bool gotEPG, requeryEPG;
    struct timespec abs_wait;
    struct timeval now;

	CInfoViewer* InfoViewer = (CInfoViewer*) arg;
	while(1)
	{
        pthread_mutex_lock( &InfoViewer->epg_mutex );
        pthread_cond_wait( &InfoViewer->epg_cond, &InfoViewer->epg_mutex );

//        printf("CInfoViewer::InfoViewerThread after pthread_cond_wait\n");

        if ( ( InfoViewer->is_visible ) )
        {
            gotEPG = true;
            repCount = 10;

            do
            {
                if ( !gotEPG )
                {
                    if ( repCount > 0 )
                        InfoViewer->showWarte();

//                    printf("CInfoViewer::InfoViewerThread before waiting long\n");
//                    usleep( 1000000 );

                    gettimeofday(&now, NULL);
                    TIMEVAL_TO_TIMESPEC(&now, &abs_wait);
                    abs_wait.tv_sec += 1;

                    pthread_mutex_trylock( &InfoViewer->epg_mutex );
                    pthread_cond_timedwait( &InfoViewer->epg_cond, &InfoViewer->epg_mutex, &abs_wait );

//                    printf("CInfoViewer::InfoViewerThread after waiting long\n");

                    repCount--;
                }

                pthread_mutex_trylock( &InfoViewer->epg_mutex );
                query = InfoViewer->CurrentChannel;
                pthread_mutex_unlock( &InfoViewer->epg_mutex );


//                printf("CInfoViewer::InfoViewerThread getEPGData for %s\n", query.c_str());

                gotEPG = InfoViewer->getEPGData(query);

                pthread_mutex_trylock( &InfoViewer->epg_mutex );

                requeryEPG = ( ( (!gotEPG) || (query!=InfoViewer->CurrentChannel) ) &&
                               ( InfoViewer->is_visible ) );

                if (query!=InfoViewer->CurrentChannel)
                    repCount = 10;

                pthread_mutex_unlock( &InfoViewer->epg_mutex );

                if ( ( !requeryEPG) && ( InfoViewer->is_visible ) )
				{
//                    printf("CInfoViewer::InfoViewerThread success\n");
					InfoViewer->showData();
				}
                else
                {
//                    printf("CInfoViewer::InfoViewerThread unsuccessful\n");
                }


            } while ( ( requeryEPG ) && (repCount > 0) );
        }

	}
	return NULL;
}


char* copyStringto( char* from, char* to, int len)
{
	while( *from != '\n' )
	{
		if (len>2)
		{
			*to = *from;
			to++;
			len--;
		}
		from++;
	}
	*to = 0;
	from ++;
	return from;
}

bool CInfoViewer::getEPGData( string channelName )
{
	#ifdef EPG_SECTIONSD
		int sock_fd;
		SAI servaddr;
		char rip[]="127.0.0.1";
		bool retval = false;

		sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		memset(&servaddr,0,sizeof(servaddr));
		servaddr.sin_family=AF_INET;
		servaddr.sin_port=htons(sectionsd::portNumber);
		inet_pton(AF_INET, rip, &servaddr.sin_addr);

		strcpy( running, "");
		strcpy( next, "");
		strcpy( runningStart, "");
		strcpy( nextStart, "");
		strcpy( runningDuration, "");
		strcpy( nextDuration, "");
		runningPercent = 0;

		if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
		{
			perror("Couldn't connect to server!");
			return false;
		}

		sectionsd::msgRequestHeader req;
		req.version = 2;
		req.command = sectionsd::currentNextInformation;
		req.dataLength = channelName.length()+1;
		write(sock_fd,&req,sizeof(req));

		char chanName[50];
		strcpy(chanName, channelName.c_str());
		for(int count=strlen(chanName)-1;count>=0;count--)
		{
			if((chanName[count]==' ') || (chanName[count]==0))
			{
				chanName[count]=0;
			}
			else
				break;
		}
		printf("query epg for >%s<\n", chanName);
		write(sock_fd, chanName, strlen(chanName)+1);

		sectionsd::msgResponseHeader resp;
		memset(&resp, 0, sizeof(resp));
		read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader));

		int nBufSize = resp.dataLength;
		if(nBufSize>0)
		{
			char* pData = new char[nBufSize+1] ;
			read(sock_fd, pData, nBufSize);
//			printf("data: %s\n\n", pData);
			char tmpPercent[10];
			char tmp[20];

			char * pos = copyStringto( pData, tmp, sizeof(tmp));
			pos = copyStringto( pos, running, sizeof(running));
			pos = copyStringto( pos, runningStart, sizeof(runningStart));
			pos = copyStringto( pos, runningDuration, sizeof(runningDuration));		
			pos = copyStringto( pos, tmpPercent, sizeof(tmpPercent));
			pos = copyStringto( pos, tmp, sizeof(tmp));
			pos = copyStringto( pos, next, sizeof(next));
			pos = copyStringto( pos, nextStart, sizeof(nextStart));
			pos = copyStringto( pos, nextDuration, sizeof(nextDuration));		

			runningPercent = atoi(tmpPercent);

			int val = atoi(runningDuration);
			sprintf((char*) &runningDuration, "%d min", val);
			val = atoi(nextDuration);
			sprintf((char*) &nextDuration, "%d min", val);


			delete[] pData;
			retval = true;
		}

		printf("exit epg-get\n\n");
		close(sock_fd);
		return retval;
	#endif
	#ifndef EPG_SECTIONSD
		strcpy( running, "");
		strcpy( next, "");
		strcpy( runningStart, "");
		strcpy( nextStart, "");
		strcpy( runningDuration, "");
		strcpy( nextDuration, "");
		runningPercent = 0;
		return true;
	#endif
}


