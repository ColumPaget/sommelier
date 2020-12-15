#ifndef ASDA_COMMON_H
#define ASDA_COMMON_H

#include "libUseful-4/libUseful.h"
#include "WMS6.1/libWMS.h"

#define ORDERS_DIR "/home/Site/Orders/"
#define AUDIT_DIR "/home/Site/Data/Today/Audit/"
#define IMPORT_LOG_PATH "/home/Site/Data/Today/LogFiles/JDAImport.dat"
#define IMPORT_DETAILS_DIR "/home/Site/Data/Today/ImportLogs/"
#define ORDERS_IMPORTED_PATH "/home/Site/Data/Today/LogFiles/ImportedOrders.dat"
#define ORDERS_PROCESSED_PATH "/home/Site/Data/Today/LogFiles/ProcessedOrders.dat"
#define ORDERS_ACTIVE_PATH "/home/Site/StaticData/ItemsInFlight.dat"
#define DELETED_ORDERS_PATH "/home/Site/Data/Today/LogFiles/DeletedOrders.dat"
#define STARTOFDAY_ORDERS_PATH "/home/Site/Data/Today/LogFiles/StartOfDayOrders.dat"
#define LABEL_PATH "/home/Site/Labels/"
#define MAN2018_LABEL_PATH "/home/man2018/"
#define LABEL_ARCHIVE_PATH "/home/Site/Data/Today/ArchiveData/Labels/"
#define LABEL_QUEUE "/home/Site/StaticData/LabelQueue.dat"

#define ITEM_DELETED  0
#define ITEM_IMPORTED 1
#define ITEM_SCANNED  2
#define ITEM_PRINTED  3
#define ITEM_CONFIRMED 4
#define ITEM_DATA_SENT 5
#define ITEM_NODATA 50
#define ITEM_FAILED 51
#define ITEM_NOTINFLIGHT 52
#define ITEM_NOLABEL 53
#define ITEM_NOREAD 54

#define IMPORT_OKAY       0
#define ERR_SEQUENCE      1
#define ERR_EMPTY         2
#define ERR_DATA          3
#define ERR_FIELDLEN      4
#define ERR_BLANKFIELD    5
#define ERR_TOOMANYFIELDS 6
#define ERR_DUPLICATE     7

#define MAXDAYS 10
#define HOURSECS 3600
#define DAYSECS  HOURSECS * 24

int OpenOrderBucket(const char *Barcode, CDataFile *DF);
int OpenOrderIndex(const char *OrderNo, CDataFile *DF);
int OpenAuditBucket(const char *Barcode, CDataFile *DF);
int UpdateAudit(int Type, const char *Barcode, const char *Event);
const char *SendPLCMessage(STREAM *S, const char *Msg);
int OpenHistoryAuditBucket(const char *Path, const char *Barcode, CDataFile *DF);
int OpenParcelBarcodeIndex(const char *ParcelBarcode, CDataFile *DF);



#endif
