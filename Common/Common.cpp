#include "Common.h"

int NoOfBuckets=4000;
#define PLC_MSG_LEN 70


int CalculateHashVal(const char *Barcode)
{
const char *ptr;
int val=0;

for (ptr=Barcode; *ptr !='\0'; ptr++)
{
val ^= ( val << 5 ) + (val >> 2 ) + *ptr;
}

return(val);
}

int OpenOrderBucket(const char *Barcode, CDataFile *DF)
{
char *Tempstr=NULL;
const char *ptr;
unsigned int i, val=0;

if (! Barcode) return(0);
/*
ptr=Barcode;
if (isalpha(*ptr)) ptr++;
val=strtol(ptr,NULL,10);
*/


val=CalculateHashVal(Barcode);


Tempstr=FormatStr(Tempstr,"%s%06d.dat",ORDERS_DIR,val % NoOfBuckets);
MakeDirPath(Tempstr,0777);

if (! DF->Open(Tempstr))
{
  DF->AddDataField("UsedFlag",1);
	DF->AddDataField("Barcode",25);
  DF->AddDataField("ParcelBarcode",30);
  DF->AddDataField("LPN",30);
  DF->AddDataField("DeliveryType",20);
  DF->AddDataField("CustomerName",50);
  DF->AddDataField("OrderNo",30);
  DF->AddDataField("StoreLocation",40);
  DF->AddDataField("MetapackConsNo",50);
  DF->AddDataField("CarrierName",50);
  DF->AddDataField("CarrierService",50);
  DF->AddDataField("Consignment",50);
  DF->AddDataField("XofY",7);
  DF->AddDataField("CarrierName",50);
  DF->AddDataField("CarrierService",50);
  DF->AddDataField("AddressLine1",75);
  DF->AddDataField("AddressLine2",75);
  DF->AddDataField("AddressLine3",75);
  DF->AddDataField("AddressLine4",40);
  DF->AddDataField("PostCode",10);
  DF->AddDataField("Aisle",4);
  DF->AddDataField("Location",14);
  DF->AddDataField("StoreNumber",5);
  DF->AddDataField("PrintedDate",11);
	DF->AddDataField("Region",50);
	DF->AddDataField("Weight",16);

  DF->AddDataField("ImportTime",25);
  DF->AddDataField("LastActivity",25);
  DF->Create(Tempstr);
  DF->Open(Tempstr);
}

DestroyString(Tempstr);

return(DF->IsOpen());
}

int OpenOrderIndex(const char *OrderNo, CDataFile *DF)
{
char *Tempstr=NULL, *ptr;
unsigned int i, val=0;

val=CalculateHashVal(OrderNo);
Tempstr=FormatStr(Tempstr,"%s%06d.orderidx",ORDERS_DIR,val % NoOfBuckets);
MakeDirPath(Tempstr,0777);

if (! DF->Open(Tempstr))
{
    DF->AddDataField("UsedFlag",1);
    DF->AddDataField("OrderNo",30);
    DF->AddDataField("Barcode",25);
    DF->Create(Tempstr);
    DF->Open(Tempstr);
}

DestroyString(Tempstr);

return(DF->IsOpen());
}


int OpenParcelBarcodeIndex(const char *ParcelBarcode, CDataFile *DF)
{
char *Tempstr=NULL, *ptr;
unsigned int i, val=0;

val=CalculateHashVal(ParcelBarcode);
Tempstr=FormatStr(Tempstr,"%s%06d.parcelidx",ORDERS_DIR,val % NoOfBuckets);
MakeDirPath(Tempstr,0777);

if (! DF->Open(Tempstr))
{
    DF->AddDataField("UsedFlag",1);
    DF->AddDataField("ParcelBarcode",30);
    DF->AddDataField("OrderNo",30);
    DF->AddDataField("Barcode",25);
    DF->Create(Tempstr);
    DF->Open(Tempstr);
}

DestroyString(Tempstr);

return(DF->IsOpen());
}



int OpenAuditBucket(const char *Barcode, CDataFile *DF)
{
char *Tempstr=NULL;
const char *ptr;
int i, val=0;

if (! Barcode) return(0);

val=CalculateHashVal(Barcode);
Tempstr=FormatStr(Tempstr,"%s%06d.dat",AUDIT_DIR,val % NoOfBuckets);
MakeDirPath(Tempstr,0777);

if (! DF->Open(Tempstr))
{
    DF->AddDataField("Type",2);
    DF->AddDataField("Barcode",25);
    DF->AddDataField("Event",80);
    DF->AddDataField("Timestamp",25);
    DF->Create(Tempstr);
    DF->Open(Tempstr);
}

DestroyString(Tempstr);

return(DF->IsOpen());
}

int UpdateAudit(int Type, const char *Barcode, const char *Event)
{
CDataFile DF;
char *Tempstr=NULL;
int i, val=0;

if (OpenAuditBucket(Barcode, &DF))
{
	DF.AddRecord();
	DF.PopulateFieldAsNum("Type",Type);
	DF.PopulateField("Barcode",Barcode);
	DF.PopulateField("Event",Event);
	DF.PopulateFieldAsDate("Timestamp");
	DF.Close();
}

DestroyString(Tempstr);
}


const char *SendPLCMessage(STREAM *S, const char *Msg)
{
static int MsgNo=0;
static char *Tempstr=NULL;
int len;

MsgNo++;
if (MsgNo > 9999) MsgNo=0;

Tempstr=FormatStr(Tempstr,"%04dCCSPLC_%s\n",MsgNo,Msg);
len=StrLen(Tempstr);

while (len < PLC_MSG_LEN)
{
  Tempstr=AddCharToStr(Tempstr,' ');
  len++;
}

//Log.SendToDebug("TOPLC: %s",Tempstr);

STREAMWriteLine(Tempstr,S);
STREAMFlush(S);
usleep(20000);


return(Tempstr);
}


int OpenHistoryAuditBucket(const char *Path, const char *Barcode, CDataFile *DF)
{
char *Tempstr=NULL;
int i, val=0;


val=CalculateHashVal(Barcode);
Tempstr=FormatStr(Tempstr,"%s%06d.dat",Path,val % NoOfBuckets);
MakeDirPath(Tempstr,0777);

if (! DF->Open(Tempstr))
{
    DF->AddDataField("Type",2);
    DF->AddDataField("Barcode",20);
    DF->AddDataField("Event",80);
    DF->AddDataField("Timestamp",25);
    DF->Create(Tempstr);
    DF->Open(Tempstr);
}

DestroyString(Tempstr);

return(DF->IsOpen());
}


