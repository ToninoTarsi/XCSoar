/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005  

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

}
*/
#include "stdafx.h"
#include "Waypointparser.h"
#include "externs.h"
#include "Dialogs.h"
#include "resource.h"
#include "Utils.h"


#include <windows.h>
#include <Commctrl.h>
#include <aygshell.h>

#include <tchar.h>

TCHAR *strtok_r(TCHAR *s, TCHAR *delim, TCHAR **lasts);

static void ExtractParameter(TCHAR *Source, TCHAR *Destination, int DesiredFieldNumber);
static int ParseWayPointString(TCHAR *TempString,WAYPOINT *Temp);
static double CalculateAngle(TCHAR *temp);
static int CheckFlags(TCHAR *temp);
static double ReadAltitude(TCHAR *temp);

static TCHAR TempString[210];


void CloseWayPoints() {
  NumberOfWayPoints = 0; 
  if(WayPointList != NULL) {
    LocalFree((HLOCAL)WayPointList);
    WayPointList = NULL;
  }
}


void ReadWayPointFile(FILE *fp)
{
  WAYPOINT *List, *p;
  TCHAR szTemp[100];
  int nTrigger=10;
  DWORD fSize, fPos=0;
  int nLineNumber=0;

  HWND hProgress;

  hProgress = CreateProgressDialog(gettext(TEXT("Loading Waypoints File...")));

  fSize = GetFileSize((void *)_fileno(fp), NULL);

  if (fSize == 0) {
    return;
  }

  // memory allocation
  if (!WayPointList) {
    WayPointList = (WAYPOINT *)LocalAlloc(LPTR, 50 * sizeof(WAYPOINT));
  }
  
  if(WayPointList == NULL) 
    {
      MessageBox(hWndMainWindow,gettext(TEXT("Not Enough Memory For Waypoints")),TEXT("Error"),MB_OK|MB_ICONSTOP);
      return;
    }

  List = WayPointList+NumberOfWayPoints;

  // SetFilePointer(hFile,0,NULL,FILE_BEGIN);
  fPos = 0;
  nTrigger = (fSize/10);  

  while(ReadStringX(fp, 200, TempString)){
    
    nLineNumber++;
    fPos += _tcslen(TempString);
    
    if (nTrigger < (int)fPos){
      nTrigger += (fSize/10);
      StepProgressDialog();
    }
    
    if(_tcsstr(TempString,TEXT("**")) != TempString) // Look For Comment
      {
        List->Details = NULL; 
        if (ParseWayPointString(TempString,List)) {

          List ++;
          NumberOfWayPoints++;

          if ((NumberOfWayPoints % 50) == 0){

            if ((p = (WAYPOINT *)LocalReAlloc(WayPointList, ((NumberOfWayPoints/50)+1) * 50 * sizeof(WAYPOINT), LMEM_MOVEABLE | LMEM_ZEROINIT)) == NULL){

              MessageBox(hWndMainWindow,gettext(TEXT("Not Enough Memory For Waypoints")),TEXT("Error"),MB_OK|MB_ICONSTOP);

              return;
            }

            if (p != WayPointList){
              WayPointList = p;
              List = WayPointList + NumberOfWayPoints;
            }

          }

        } else {

          _stprintf(szTemp, gettext(TEXT("Waypointfile Parse Error at Line %d")), nLineNumber);
          MessageBox(hWndMainWindow,szTemp,TEXT("Error"),MB_OK | MB_ICONWARNING);

        }
      }
  }

  wsprintf(szTemp,TEXT("100%%"));       
  SetDlgItemText(hProgress,IDC_PROGRESS,szTemp);

}



int ParseWayPointString(TCHAR *TempString,WAYPOINT *Temp)
{
  TCHAR ctemp[80];
  TCHAR *Zoom;
  TCHAR *pWClast = NULL;
  TCHAR *pToken;


  // ExtractParameter(TempString,ctemp,0);
  if ((pToken = strtok_r(TempString, TEXT(","), &pWClast)) == NULL)
    return FALSE;
  Temp->Number = _tcstol(pToken, &Zoom, 10);
        
  //ExtractParameter(TempString,ctemp,1); //Latitude
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL)
    return FALSE;
  Temp->Latitude = CalculateAngle(pToken);

  if((Temp->Latitude > 90) || (Temp->Latitude < -90))
    {
      return FALSE;
    }

  //ExtractParameter(TempString,ctemp,2); //Longitude
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL)
    return FALSE;
  Temp->Longitude  = CalculateAngle(pToken);
  if((Temp->Longitude  > 180) || (Temp->Longitude  < -180))
    {
      return FALSE;
    }

  //ExtractParameter(TempString,ctemp,3); //Altitude
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL)
    return FALSE;
  Temp->Altitude = ReadAltitude(pToken);
  if (Temp->Altitude == -9999){
    return FALSE;
  }

  //ExtractParameter(TempString,ctemp,4); //Flags
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL)
    return FALSE;
  Temp->Flags = CheckFlags(pToken);

  //ExtractParameter(TempString,ctemp,5); // Name
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL)
    return FALSE;

  // guard against overrun
  if (_tcslen(pToken)>NAME_SIZE) {
    pToken[NAME_SIZE-1]= _T('\0');
  }

  _tcscpy(Temp->Name, pToken);
  int i;
  for (i=_tcslen(Temp->Name)-1; i>1; i--) {
    if (Temp->Name[i]==' ') {
      Temp->Name[i]=0;
    } else {
      break;
    }
  }

  //ExtractParameter(TempString,ctemp,6); // Comment
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) != NULL){
    _tcscpy(ctemp, pToken);
    ctemp[COMMENT_SIZE] = '\0';

    Temp->Zoom = 0;
    Zoom = _tcschr(ctemp,'*');
    if(Zoom)
      {
        *Zoom = '\0';
        Zoom +=2;
        Temp->Zoom = _tcstol(Zoom, &Zoom, 10);
      }

    // sgi, move "panic-stripping" of the comment-field after we extract
    // the zoom factor
    ctemp[COMMENT_SIZE] = '\0';
    _tcscpy(Temp->Comment, ctemp);
  } else {
    Temp->Comment[0] = '\0';
    Temp->Zoom = 0;
  }

  if(Temp->Altitude == 0){

    LockTerrainDataGraphics();
    terrain_dem_graphics.SetTerrainRounding(0.0);
    double myalt =
      terrain_dem_graphics.GetTerrainHeight(Temp->Latitude , Temp->Longitude);
    UnlockTerrainDataGraphics();

    Temp->Altitude = myalt;

  }

  if (Temp->Details) {
    free(Temp->Details);
  }

  return TRUE;
}

  
void ExtractParameter(TCHAR *Source, TCHAR *Destination, int DesiredFieldNumber)
{
  int index = 0;
  int dest_index = 0;
  int CurrentFieldNumber = 0;
  int StringLength        = 0;

  StringLength = _tcslen(Source);

  while( (CurrentFieldNumber < DesiredFieldNumber) && (index < StringLength) )
    {
      if ( Source[ index ] == ',' )
        {
          CurrentFieldNumber++;
        }
      index++;
    }

  if ( CurrentFieldNumber == DesiredFieldNumber )
    {
      while( (index < StringLength)    &&
             (Source[ index ] != ',') &&
             (Source[ index ] != 0x00) )
        {
          Destination[dest_index] = Source[ index ];
          index++; dest_index++;
        }
      Destination[dest_index] = '\0';
    }
  // strip trailing spaces
  for (int i=dest_index-1; i>0; i--) {
    if (Destination[i]==' ') {
      Destination[i]= '\0';
    } else return;
  }
}

static double CalculateAngle(TCHAR *temp)
{
  TCHAR *Colon;
  TCHAR *Stop;
  double Degrees, Mins;

  Colon = _tcschr(temp,':');

  if(!Colon)
    {
      return -9999;
    }

  *Colon =  NULL;
  Colon ++;

  Degrees = (double)_tcstol(temp, &Stop, 10);
  Mins = (double)StrToDouble(Colon, &Stop);
  if (*Stop == ':') {
    Mins += ((double)_tcstol(++Stop, &Stop, 10)/60.0);
  }

  Degrees += (Mins/60);
        
  if((*Stop == 'N') || (*Stop == 'E'))
    {
    }
  else if((*Stop == 'S') || (*Stop == 'W'))
    {
      Degrees *= -1;
    }
  else
    {
      return -9999;
    }
        
  return Degrees;
}

static int CheckFlags(TCHAR *temp)
{
  int Flags = 0;

  if(_tcschr(temp,'A')) Flags += AIRPORT;
  if(_tcschr(temp,'T')) Flags += TURNPOINT;
  if(_tcschr(temp,'L')) Flags += LANDPOINT;
  if(_tcschr(temp,'H')) Flags += HOME;
  if(_tcschr(temp,'S')) Flags += START;
  if(_tcschr(temp,'F')) Flags += FINISH;
  if(_tcschr(temp,'R')) Flags += RESTRICTED;
  if(_tcschr(temp,'W')) Flags += WAYPOINTFLAG;

  return Flags;
}


static double ReadAltitude(TCHAR *temp)
{
  TCHAR *Stop;
  double Altitude=-9999;


  Altitude = (double)_tcstol(temp, &Stop, 10);

  if (temp == Stop)                                         // error at begin
    Altitude=-9999;
  else {
    if (Stop != NULL){                                      // number converted endpointer is set

      switch(*Stop){

        case 'M':                                           // meter's nothing to do
        case 'm':
        case '\0':
        break;

        case 'F':                                           // feet, convert to meter
        case 'f':
          Altitude = Altitude / TOFEET;
        break;

        default:                                            // anything else is a syntax error   
          Altitude = -9999;
        break;

      }
    }
  }

  return Altitude;
}

extern TCHAR szRegistryWayPointFile[];  
extern TCHAR szRegistryAdditionalWayPointFile[];  

void ReadWayPoints(void)
{
  TCHAR szFile1[MAX_PATH] = TEXT("\0");
  TCHAR szFile2[MAX_PATH] = TEXT("\0");
        
  FILE *fp;

  LockFlightData();
  CloseWayPoints();
  GetRegistryString(szRegistryWayPointFile, szFile1, MAX_PATH);
  SetRegistryString(szRegistryWayPointFile, TEXT("\0"));  
      
  fp = _tfopen(szFile1, TEXT("rt"));
                        
  if(fp != NULL)
    {
      ReadWayPointFile(fp);
      fclose(fp);
      // read OK, so set the registry to the actual file name
      SetRegistryString(szRegistryWayPointFile, szFile1);  
    }

  // read additional waypoint file

  GetRegistryString(szRegistryAdditionalWayPointFile, szFile2, MAX_PATH);
  SetRegistryString(szRegistryAdditionalWayPointFile, TEXT("\0"));  

  fp = _tfopen(szFile2, TEXT("rt"));
  if(fp != NULL)
    {
      ReadWayPointFile(fp);
      fclose(fp);
      // read OK, so set the registry to the actual file name
      SetRegistryString(szRegistryAdditionalWayPointFile, szFile2);  
    }

  UnlockFlightData();

}


void SetHome(void)
{
  TCHAR szRegistryHomeWaypoint[]= TEXT("HomeWaypoint");
  unsigned int i;

  // check invalid home waypoint
  if((HomeWaypoint <0)||(HomeWaypoint >= (int)NumberOfWayPoints))
    {
      HomeWaypoint = -1;
    }

  // search for home in waypoint list
  for(i=0;i<NumberOfWayPoints;i++)
    {
      if( (WayPointList[i].Flags & HOME) == HOME)
          {
            if (HomeWaypoint== -1) {
              HomeWaypoint = i;
            }
          }
    }

//
// If we haven't found the waypoint or the homewaypoint is out of
// range then set it to the first in the waypoint list
//
  if (HomeWaypoint<0) {
    HomeWaypoint = 0;
  }

  // Assume here we have a home now...
  GPS_INFO.Latitude = WayPointList[HomeWaypoint].Latitude;
  GPS_INFO.Longitude = WayPointList[HomeWaypoint].Longitude;


  // 
  // Save the home waypoint number in the resgistry
  //
  SetToRegistry(szRegistryHomeWaypoint,HomeWaypoint);
}


int FindNearestWayPoint(double X, double Y, double MaxRange)
{
  unsigned int i;
  int NearestIndex = 0;
  double NearestDistance, Dist;

  if(NumberOfWayPoints ==0)
    {
      return -1;
    }
                
  NearestDistance = Distance(Y,X,WayPointList[0].Latitude, WayPointList[0].Longitude);
  for(i=1;i<NumberOfWayPoints;i++)
    {
      if (((WayPointList[i].Zoom >= MapWindow::MapScale*10)||
           (WayPointList[i].Zoom == 0))
          &&(MapWindow::MapScale <= 10)) {

            // only look for visible waypoints
            // feature added by Samuel Gisiger
            Dist = Distance(Y,X,WayPointList[i].Latitude, WayPointList[i].Longitude);
            if(Dist < NearestDistance)
              {
                NearestIndex = i;
                NearestDistance = Dist;
              }
          }
    }
  if(NearestDistance < MaxRange)
    return NearestIndex;
  else
    return -1;
}





