#ifndef _STAT_RECORDS_
#define _STAT_RECORDS_

#include <Arduino.h>
//#include "FS.h"
//#include "SPIFFS.h"
#include "LITTLEFS.h"
//#include "display.hpp"
#include <TimeLib.h>

struct statRecord_t{
    time_t   Time;
    uint32_t Steps;
    float    HeartRate;
    float    SpO2;
    float    AmbientTempC;
    float    ObjectTempC;
    float    Vcc;
};
  
#define  CACHE_RECORD_CNT 10

extern statRecord_t  RTC_RECORDS[CACHE_RECORD_CNT];
extern uint8_t statRecord_cnt;


#define SPIFFS LITTLEFS

#define HOURS_IN_DAY  24

typedef enum record_idx_s{
        REC_TIME_IDX  = 0,
        REC_VCC_IDX,
        REC_STEPS_IDX,
        REC_HR_IDX,
        REC_AMBT_IDX,
        REC_OBJT_IDX,
        REC_MAX_IDX
}  record_idx_t;

enum skimrecord_idx_t{
     //   SKIMREC_TIME_IDX  = TIME_IDX,
        SKIMREC_VCC_IDX,
        SKIMREC_STEPS_IDX,
        SKIMREC_OBJT_IDX,
        SKIMREC_MAX_IDX
};

enum result_aggregate_t {
      RESULT_AGGREGATE_AVG = 0, 
      RESULT_AGGREGATE_SUM,
      RESULT_AGGREGATE_DIFF,
};

struct HourRecordAvg  {
    private:        
        float   startValue;
        float   endValue;
        float   summ;
        uint8_t cnt; // we have only 60 recors in total;

    public:
        HourRecordAvg();
        void add(float val);
        void addBack(float val);

        float avg();
        float diff();
        float diffAsc();
        float sum();

        void print();
};

int       buildDayFName(char *inputBuffer, int inputBuffer_size, char *dir, int16_t year, int8_t month, int8_t  day);
uint32_t  getArrayLogfiles(time_t lastKnownData, int32_t *resData, uint32_t resDataSize);
tmElements_t toTmElement(struct tm *tm);

struct skimData_t {
    float     Vcc;
    uint32_t  Steps;
    float     ObjectTempC;

    void fill_data(float vcc , uint32_t steps , float objC );
};

class fileRecord {
    protected:
        struct tm   _tm;
        void       *_data ;
        uint8_t     max_cnt; 
        uint8_t     field_cnt;
        virtual void castData(char **dataPointers) ;    

    public:
//        fileRecord();
  //      ~fileRecord();
        bool        parseLine(char *_buffer, char delimiter);
        int         hour() { return _tm.tm_hour; };
        struct tm  *tm()   { return &_tm;};
        
};

class FileRecord: public fileRecord {  //  i don't want to parse and cast all data,
    private:        
        void castData(char **dataPointers) ;    

    public:
        FileRecord();
        ~FileRecord();
        statRecord_t *data() {return (statRecord_t *) _data;};
        
    
};

class DailyRecordsAvg {
    private:        
        HourRecordAvg val[HOURS_IN_DAY];
        uint8_t       cnt;  // records since 0th hour
    public:
        DailyRecordsAvg();
        int  aggregateString(uint8_t _hour, float value);
        int  aggregateStringBack(uint8_t _hour, float value);
        HourRecordAvg *  hourRecord(uint8_t hourIdx);
};

class SkimFileRecord: public fileRecord {  /// TODO: make  parent common with  FileRecord
    private:        
    //    uint8_t     max_cnt = SKIMREC_MAX_IDX;
       // skimData_t *_data = NULL;
        void castData(char **dataPointers) ;
        bool needWrite;
    public:
        SkimFileRecord();
        ~SkimFileRecord();
        
        skimData_t *get();
        void         set( skimData_t *s );
        float        getField( skimrecord_idx_t idx );

        void setWritable();
        bool isWritable();
        bool serialize(char *str, size_t str_size);
        void print();
};

struct rawForSkim_t {
    DailyRecordsAvg  Steps;
    DailyRecordsAvg  Temp;
    DailyRecordsAvg  Vcc;
    int8_t    startHourInLog;
    int8_t    endHourInLog;
};

struct hourStat_t {
        SkimFileRecord   byHours[HOURS_IN_DAY];
        int8_t           hoursInFile;  // -1 in case not inited
        int8_t           hoursInTotal; 
};

class SkimData {
    private:
        rawForSkim_t *r4s;
 //       char *skimDirName   = "/skim";
 
        uint8_t maxFilesCnt = 30;
        uint8_t _day;
        uint8_t _month; 
        uint16_t _year; // 64k 

        hourStat_t      stat;

        bool isCurrentHour(uint8_t _hour);
        bool isToday();

        bool processRaw();
        void prepareFromLog();  // fill r4s // in case it happened today
        bool prepareFromMemory();

        bool processLine(char *lineString);

        bool aggreggateRecord(int _hour, statRecord_t *stat_rec);
        uint8_t read();
        void    write();
                
    public: 
        SkimData(uint8_t d, uint8_t m , uint16_t y);
        ~SkimData();

        void process(); 
        hourStat_t *getStat();
};


typedef void (*readFileHandler)(char* fname);

#define MAX_FILES_CNT 10
#define MAX_LINE_SIZE 128
#define START_GOOD_DATE 1600000000  // some test for data

class FileSystem {
    private:
        bool    _canWrite =  true; //false;
/*
        FileRecord    skim_frec[HOURS_IN_DAY];
        uint8_t       skim_frec_cnt;
  */      
        File    _file;
        char *logDirName     = "/log";

        char *filenamePattern = "%04d-%02d-%02d.txt";
        
        void saveRecordsToFile();
        void listDir(fs::FS &fs, const char * dirname, bool need_cat, readFileHandler r_handler );
    public:
        void init();
        void catFile(File f);
        void writeLog(statRecord_t *record);
        void scanLogDir();
        void rotateLogs();
        void close();

        int filenameDate(char *inputBuffer, int inputBuffer_size, int16_t year, int8_t month, int8_t  day);


        ///int8_t get_values_idx(time_t _date,  record_idx_t field_idx, float result_data[24] );
       // int8_t get_values_idx(int32_t tm_mday, int32_t tm_month, int32_t tm_year,  record_idx_t field_idx,
         //                     result_aggregate_t result_type, Graph *graph ) ;

};

#endif
