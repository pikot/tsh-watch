#include "stat_records.hpp"
#include "utils.h"
#include <TimeLib.h>

RTC_DATA_ATTR statRecord_t RTC_RECORDS[CACHE_RECORD_CNT];
RTC_DATA_ATTR uint8_t      statRecord_cnt = 0;


char *logFilenamePattern = "%04d-%02d-%02d.txt";
char *logDirName  = "/log";
char *skimDirName = "/skim";

tmElements_t toTmElement(struct tm *tm)
{
    tmElements_t tme;
    tme.Second = tm->tm_sec;
    tme.Minute = tm->tm_min;
    tme.Hour = tm->tm_hour;
    tme.Wday = tm->tm_wday + 1;   // day of week, sunday is day 1
    tme.Day = tm->tm_mday;
    tme.Month = tm->tm_mon + 1;  // Jan = 1 in tme, 0 in tm
    tme.Year = (uint8_t)(tm->tm_year - 70); // offset from 1970;
    return tme;
}

int buildDayFName(char *inputBuffer, int inputBuffer_size, char *dir, int16_t year, int8_t month, int8_t  day) 
{
    char patternStr[64];
    snprintf(patternStr, sizeof(patternStr), logFilenamePattern, year, month, day);  
    return snprintf(inputBuffer, inputBuffer_size,"%s/%s", dir, patternStr); 
}

int fidCompare(const void *i, const void *j)
{
    return *(int32_t *)i - *(int32_t *)j;
}

time_t setZeroHoursToTime(time_t _date) 
{
    struct tm *tmstruct   = localtime(&_date); 
    tmstruct->tm_hour = 0;
    tmstruct->tm_min = 0;
    tmstruct->tm_sec = 0;
    tmElements_t tme = toTmElement(tmstruct);
    return makeTime(tme);
}

uint32_t calcNumDays(time_t date1, time_t date2)
{
    time_t d1 = setZeroHoursToTime(date1);
    time_t d2 = setZeroHoursToTime(date2);
    return  1 + ((d1 - d2) / 86400);
}

uint32_t getArrayLogfiles(time_t lastKnownData, int32_t *resData, uint32_t resDataSize)
{
    char  *dirName = logDirName;
    char  *filenamePattern = logFilenamePattern;
    char    fPattern[64];
    int32_t file_cnt = 0;
    int32_t y=0, m=0, d=0;
    int32_t dates[20];
    int32_t cur_date;
    
    printf_w("get_arrayfiles: start\n");
    snprintf(fPattern, sizeof(fPattern), "%s/%s", dirName, filenamePattern); 
    printf_w("get_arrayfiles: scan %s\n", fPattern );

    File root = SPIFFS.open(dirName);
    File file = root.openNextFile();
    while (file) {
        if (file_cnt >= sizeof(dates)) 
            break;
        
        if (file.isDirectory()) {
            file = root.openNextFile();
            continue;
        } else {
             sscanf(file.name(), fPattern, &y, &m, &d );
             dates[file_cnt] = y * 10000 + m*100 + d;
             file_cnt += 1;
        }
        file = root.openNextFile();
    }
    qsort(dates, file_cnt, sizeof(int32_t), fidCompare);
    printf_w("get_arrayfiles: file_cnt %d\n", file_cnt );

/*
    for (int32_t i = 0; i < file_cnt ; i++ ) 
          printf_w("get_arrayfiles: date %d\n", dates[i] );
    
*/
    int32_t diffDay =  calcNumDays(now(), lastKnownData);
    if (diffDay > file_cnt)
          diffDay = file_cnt;
    printf_w("get_arrayfiles: diffDay %d\n", diffDay);

    int32_t  startIdx = file_cnt - diffDay;    
    int32_t idx = 0;
    for (int32_t i = 0; i < diffDay; i++) {
            if (i >= resDataSize) 
                break;
            
            idx = startIdx + i;
            printf_w("--- res to send -- %d\n", dates[idx] );
            resData[i] = dates[idx];
    }

    printf_w("get_arrayfiles: stop\n");
    return diffDay;
}

void  _rotateLogs( char * dirName, char *filenamePattern, int maxFilesCnt) 
{
    File root = SPIFFS.open(dirName);
    if (!root) {
        printf_w("rotateLogs: can't open dir '%s'\n", dirName);
        return;
    }
    if(!root.isDirectory()){
        printf_w("rotateLogs: root is not dir '%s'\n", dirName);
        return;
    }
    int32_t y=0, m=0, d=0;
    int32_t min_date = 30000000, cur_date;
    int32_t file_cnt = 0;
    char fPattern[64];
    
    int32_t dates[20];
    snprintf(fPattern, sizeof(fPattern), "%s/%s", dirName, filenamePattern); 
    File file = root.openNextFile();
    while (file) {
        if (file_cnt == sizeof(dates))
            break;
                  
        if (file.isDirectory()) 
            continue;
        else {
             sscanf(file.name(), fPattern, &y, &m, &d );
             cur_date = y * 10000 + m*100 + d;
             dates[file_cnt] = cur_date;
             file_cnt += 1;
        }
        file = root.openNextFile();
    }
    
    if (file_cnt > maxFilesCnt) {
        qsort (dates, file_cnt, sizeof(int32_t), fidCompare);
        int32_t cnt_del = file_cnt - maxFilesCnt; 
        for (int32_t i = 0; i < cnt_del; i++) {
            min_date = dates[i];
            char path[32];
            int d = min_date % 100;
            int m = (min_date / 100)  % 100;
            int y = min_date / 10000;
            buildDayFName(path, sizeof(path), dirName, y, m, d);

          //  printf_w("delete , %s\n", path);
            SPIFFS.remove(path);
        }
    }
/*
    char *to_del = "/log/2094-09-20.txt";
    if ( SPIFFS.exists(to_del) )
        SPIFFS.remove(to_del);
*/
}

//-- 
HourRecordAvg::HourRecordAvg() 
{
    startValue = 0;
    endValue   = 0;
    summ        = 0;
    cnt         = 0;
}

void HourRecordAvg::add(float value)
{
    if (0 == cnt) 
        startValue = value;
    
    summ += value;
    cnt += 1;
    endValue = value;
}

void HourRecordAvg::addBack(float value)
{
    if (0  == cnt) 
        endValue = value;
    
    summ += value;
    cnt += 1;
    startValue = value;
}

float HourRecordAvg::avg()  
{ 
    return (summ/ cnt);
}

float HourRecordAvg::diffAsc() 
{ 
    if (startValue > endValue) {
           printf_w("startValue %d, endValue %d, sould asc", startValue, endValue);
           return 0;
    }
    return (endValue - startValue);
}

float HourRecordAvg::diff() 
{ 
    return (endValue - startValue);
}

float HourRecordAvg::sum() 
{ 
    return summ;
}

void HourRecordAvg::print() 
{ 
      printf_w("HourRecordAvg: startValue %.2f, endValue %.2f, summ %.2f, cnt %d\n", startValue, endValue, summ, cnt);
}

//-- 
DailyRecordsAvg::DailyRecordsAvg() 
{
          //avg_hour_record val[HOURS_IN_DAY];
}

HourRecordAvg * DailyRecordsAvg::hourRecord(uint8_t hourIdx) 
{ 
    if (hourIdx >= HOURS_IN_DAY)
        return NULL;
    return &val[hourIdx]; 
}

int DailyRecordsAvg::aggregateString(uint8_t hourIdx, float value)
{
    if (hourIdx >= HOURS_IN_DAY)
        return NULL;
    val[ hourIdx].add(value);
    return hourIdx;
}

int DailyRecordsAvg::aggregateStringBack(uint8_t hourIdx, float value)
{
    if (hourIdx >= HOURS_IN_DAY)
        return -1;
    val[hourIdx].addBack(value);

    return hourIdx;
}

void fileRecord::castData(char **dataPointers) 
{      
}

bool fileRecord::parseLine(char *_buffer, char delimiter) 
{ // ahtung - spoil _buffer!
    if (!_buffer)
        return false;
    
    bool nextIsNewData = true;
    int32_t _buffer_len = strlen(_buffer);
    int32_t field_cnt = 0;
    
    char **dataPointers = (char **) calloc(max_cnt, sizeof(char*) );
    for (int i = 0; i < _buffer_len ; i++) {
        if (nextIsNewData) {
            if (field_cnt < max_cnt) 
                dataPointers[field_cnt] = & _buffer[i];
            field_cnt++;
            nextIsNewData = false;
        }
        if (delimiter == _buffer[i]) {
            _buffer[i] = 0;
            nextIsNewData = true;
        }
    }

    if (field_cnt != max_cnt) {
        free(dataPointers);
        return false; 
    }

    
    if (!_data) {
        free(dataPointers);
        return false;
    }
    // aaand cast  
    castData(dataPointers);
    free(dataPointers);
    return true;
} 


SkimFileRecord::SkimFileRecord()
{
    max_cnt = SKIMREC_MAX_IDX;
    _data = (skimData_t*) calloc(1, sizeof(skimData_t));
    needWrite = false;
}

SkimFileRecord::~SkimFileRecord()
{
    free(_data) ;
}

void SkimFileRecord::castData(char **dataPointers) 
{
    if (!_data) {
          print_w("SkimFileRecord::castData: empty data");
          return;
    }       
    skimData_t* _d  = (skimData_t*)_data;

    _d->Vcc         = atof(dataPointers[SKIMREC_VCC_IDX]);
    _d->Steps       = atoi(dataPointers[SKIMREC_STEPS_IDX]);
    _d->ObjectTempC = atof(dataPointers[SKIMREC_OBJT_IDX]);
}

        
float SkimFileRecord::getField(skimrecord_idx_t idx)
{
      float ret;
      skimData_t* _d  = (skimData_t*)_data;

      switch(idx) {
          case  SKIMREC_VCC_IDX:
                ret =  _d->Vcc;
                break;
          case  SKIMREC_STEPS_IDX:
                ret =  (float) _d->Steps; 
                break;
          case  SKIMREC_OBJT_IDX:
                ret =  _d->ObjectTempC;
                break;
          default :
                ret = 0;
      }
      return ret;
}


skimData_t *SkimFileRecord::get()
{
    return (skimData_t*)_data;
}

void SkimFileRecord::set(skimData_t *s) 
{
    if (!_data){
          print_w("SkimFileRecord::set: empty data, it nonsence");
          return;
    }
    
    memcpy( _data, s, sizeof(skimData_t));
}

void SkimFileRecord::setWritable()
{
    needWrite = true;          
}

bool SkimFileRecord::isWritable()
{
    return  needWrite;  
}

bool SkimFileRecord::serialize(char *str, size_t str_size)
{
    if (!str) 
        return false;

    skimData_t* _d  = (skimData_t*)_data;
    snprintf(str, str_size, "%.2f\t%d\t%.2f\n", 
               _d->Vcc, _d->Steps, _d->ObjectTempC);
    return true;
}

void SkimFileRecord::print()
{  
    skimData_t* _d  = (skimData_t*)_data;
    printf_w("%.2f\t%d\t%.2f\n", 
               _d->Vcc, _d->Steps, _d->ObjectTempC);
}

FileRecord::FileRecord()
{
    max_cnt = REC_MAX_IDX;
    _data   = (statRecord_t*) calloc(1, sizeof(statRecord_t));
}

FileRecord::~FileRecord()
{
    free(_data) ;
}

void FileRecord::castData(char **dataPointers) 
{
    statRecord_t* _d  = (statRecord_t*)_data;
    _d->Time         = atoi(dataPointers[REC_TIME_IDX]);
    _d->Steps        = atoi(dataPointers[REC_STEPS_IDX]);
    _d->HeartRate    = atof(dataPointers[REC_HR_IDX]);        
    _d->AmbientTempC = atof(dataPointers[REC_AMBT_IDX]);
    _d->ObjectTempC  = atof(dataPointers[REC_OBJT_IDX]);
    _d->Vcc          = atof(dataPointers[REC_VCC_IDX]);

    time_t rec_date = _d->Time;
    struct tm *tmstruct   = localtime(&rec_date); 
    memcpy(&_tm, tmstruct, sizeof(struct tm));
  //  printf_w("FileRecord::castData: time_stamp %d, %d.%d.%d %d\n", rec_date,  (_tm.tm_year) + 1900, (_tm.tm_mon) + 1, _tm.tm_mday, _tm.tm_hour );
}

//============================================================================================
//hourly stat for list of sensors (for showing graph)
SkimData::SkimData(uint8_t d, uint8_t m , uint16_t y) 
{
    _day = d;
    _month = m;
    _year = y;
    r4s = NULL;
}

SkimData::~SkimData()
{
    if (r4s)
        free(r4s);
}

uint8_t SkimData::read()
{
    char fname[64];
    char lineString[255];
    
    File root = SPIFFS.open(skimDirName);
    if (!root)  
        SPIFFS.mkdir(skimDirName);
    else        
        root.close();
    
    buildDayFName(fname, sizeof(fname), skimDirName,  _year , _month, _day);
    printf_w("SkimData::read fname %s\n",  fname );

    File in_file = SPIFFS.open(fname, "r");   
    if (in_file.size() > 560) {
        printf_w("SkimData::read fname %s too big\n",  fname );
        in_file.close();
        SPIFFS.remove(fname);  
        stat.hoursInFile = -1;  
        return stat.hoursInFile;
    }
    uint8_t line = 0;
    bool  parse_error = false;
    while (in_file.available()) {
        String buffer = in_file.readStringUntil('\n');
        if (0 == line) { // skip version
            line++;
            continue;
        }
        lineString[0] = '\0';
        buffer.toCharArray(lineString, sizeof(lineString) );
        stat.byHours[ line - 1 ].parseLine(lineString, '\t');
        printf_w("'%d' -- '%s'\n", line - 1, lineString);
        if (line - 1 > 23) {
            printf_w("SkimData::read: alarm! WTF! %d records in skim file", line - 1);
            parse_error = true;
            break;
        }
        line++;
    }
    in_file.close();

    if (!parse_error)
        stat.hoursInFile = (line > 0) ? (line - 1) : -1;
    else {
        SPIFFS.remove(fname);  
        stat.hoursInFile = -1; 
    }
    stat.hoursInTotal = stat.hoursInFile;
    printf_w("SkimData::read fname %s, hoursInFile %d \n",  fname, stat.hoursInFile );

    return stat.hoursInFile;
}

void SkimData::write()
{
    char fname[64];
    char lineString[255];
  //  return;////////
    buildDayFName(fname, sizeof(fname), skimDirName, _year , _month, _day);
    printf_w("SkimData::write, try to write %s \n", fname);

    char *modifier = "w";
    if (SPIFFS.exists(fname) )
        modifier = "a";
     
    File in_file  = SPIFFS.open(fname, modifier);
    if (!in_file) 
        printf_w("file open failed %s\n", fname); 
    
    if ("w" == modifier)
        in_file.print("V:1\n");
     //   _rotateLogs( skimDirName, logFilenamePattern, maxFilesCnt);
    
    for (int h = 0; h < HOURS_IN_DAY; h++  ) {
        if (stat.byHours[h].isWritable()) {
              stat.byHours[h].serialize(lineString, sizeof(lineString) );
              in_file.print(lineString);
              printf_w("SkimData::write: %d --- '%s'\n", h, lineString);
        }
   //     stat.byHours[h].set(&new_record);
    }
    
    in_file.close();
}

bool SkimData::processLine(char *lineString) 
{
    FileRecord fr;
    bool parsed = fr.parseLine(lineString, '\t');
    if (!parsed) {
        printf_w("WTF! bad log format\n");
        return false;
    }

    struct tm *_rec = fr.tm();
    if (_day   != _rec->tm_mday || 
         _month != (_rec->tm_mon  + 1) || 
         _year  != (_rec->tm_year + 1900)) {
        printf_w("skip line, date in file %d-%d-%d, fname ' %d-%d-%d'\n",
                   _rec->tm_mday , _rec->tm_mon + 1 , _rec->tm_year + 1900,
                   _day, _month, _year);
        return true;
    }

    if (stat.hoursInFile > fr.hour())  {
        printf_w("Stop Scan %d reached, fr.hour %d\n", stat.hoursInFile, fr.hour() );    
        return false;
    }

    if (!r4s)
        r4s = (rawForSkim_t*) calloc (1, sizeof(rawForSkim_t));
   
    statRecord_t *stat_rec = fr.data();
    if (stat_rec &&
        (0 != stat_rec->ObjectTempC && 0 != stat_rec->Vcc)
       ) 
        aggreggateRecord(fr.hour() , stat_rec) ;
  
    return true;
}

bool SkimData::aggreggateRecord(int _hour, statRecord_t *stat_rec) 
{
    r4s->Steps.aggregateStringBack( _hour , stat_rec->Steps);
    r4s->Temp.aggregateStringBack(  _hour , stat_rec->ObjectTempC);
    r4s->Vcc.aggregateStringBack(  _hour , stat_rec->Vcc);

    r4s->startHourInLog = _hour;
    if (0 == r4s->endHourInLog) 
         r4s->endHourInLog = _hour;
}

bool SkimData::prepareFromMemory() 
{
    if (statRecord_cnt > 0 && !r4s)
        r4s = (rawForSkim_t*) calloc (1, sizeof( rawForSkim_t) );
    
    for (uint8_t i = statRecord_cnt; i > 0 ; i--) {
        if (!RTC_RECORDS[i-1].Time )
            continue;
        printf_w("SkimData::prepareFromMemory i %d, %d \n",  
              i, RTC_RECORDS[i-1].Time);

        struct tm *_tm  = localtime(&RTC_RECORDS[i-1].Time);
        aggreggateRecord( _tm->tm_hour, &RTC_RECORDS[i-1]);
    }
}

void SkimData::prepareFromLog() 
{
    char fname[64];
    char lineString[255];

    buildDayFName(fname, sizeof(fname), logDirName,  _year , _month, _day);
    
    File f = SPIFFS.open(fname, "r");   

    long pos = f.size(), line_pos = 0;
    
    char buf[MAX_LINE_SIZE];
    if (0 == pos) {
        return;
    }
    printf_w("SkimData::prepareFromLog fname %s, pos %d\n",  fname, pos ); // 

    line_pos = MAX_LINE_SIZE - 1;
    buf[ line_pos ] = 0;
    line_pos       -= 1;

    char   *dataPointers[REC_MAX_IDX];
    pos -= 1; // skip eof

    while (true) {
        if (-1 == pos) 
             break;
             
        f.seek(pos);
        char _byte = f.peek();
        if (-1 == _byte) 
            break;
        if ('\n' == _byte  &&  line_pos == MAX_LINE_SIZE - 2) { // 1st char since end
            pos -= 1;
            continue;
        }
       // printf_w("SkimData::prepareFromLog fname %s, _byte %d \n" ,fname ,  _byte);


        if (0 == line_pos) { //
            pos -= 1;
            continue;
        }

        buf[line_pos] = _byte;

        if ('\n' == _byte) {
              char *lineString =  &buf[line_pos + 1];
           //   printf_w("SkimData::prepareFromLog fname %s, get line '%s', pos %d, line_pos %d \n",  fname, lineString, pos, line_pos); // 
              if (false == processLine(lineString))
                    break;

              line_pos = MAX_LINE_SIZE - 1;
        }

        line_pos -= 1;
        pos      -= 1;
    }
}

bool SkimData::isCurrentHour( uint8_t _hour)
{
    if ((day() == _day) && 
        (month() == _month) && 
        (year() == _year) && 
        (hour() == _hour)) // use time lib, uuuuglyyyyy
       return true;

    return false;
}

bool SkimData::isToday( )
{
   if ((day() == _day) && 
        (month() == _month) && 
        (year() == _year) 
        ) // use time lib, uuuuglyyyyy
       return true;

    return false;
}

void skimData_t::fill_data(float vcc , uint32_t steps , float objC)
{
      Vcc         = vcc;
      Steps       = steps;
      ObjectTempC = objC;
}

bool SkimData::processRaw() 
{
    printf_w("SkimData::processRaw r4s->startHourInLog %d,  r4s->endHourInLog %d, hoursInFile %d\n",  
              r4s->startHourInLog,  r4s->endHourInLog, stat.hoursInFile);
    int start_hour = r4s->startHourInLog;
    
    if (r4s->startHourInLog > stat.hoursInFile + 1)
        start_hour = stat.hoursInFile + 1;
    
    for (int h = start_hour; h <= r4s->endHourInLog; h++) {
        skimData_t new_record;

        if (r4s->startHourInLog > h)
            new_record.fill_data(0, 0, 0);
        else {         
            HourRecordAvg *_s = r4s->Steps.hourRecord(h); 
            HourRecordAvg *_t = r4s->Temp.hourRecord(h);
            HourRecordAvg *_v = r4s->Vcc.hourRecord(h);
            if (!_s || ! _t || !_v ) {
                printf_w( "alarm! can't take data for hour %d, date %02d.%02d%.04d. It should be ready. Stop process.\n", h, _day, _month, _year);
                break;
            }
   //      _s->print();_t->print(); _v->print();
            new_record.fill_data(_v->avg(), (int)_s->diffAsc(), _t->avg()); 
        }
        printf_w("SkimData::processRaw: h %d, new_record(Steps %d, ObjectTempC %f, Vcc %f)\n",  
                  h, new_record.Steps , new_record.ObjectTempC,  new_record.Vcc );
        stat.byHours[h].set(&new_record);
        if (!isCurrentHour(h)) { // safe if it is not last hour today
             stat.byHours[h].setWritable();
             printf_w("SkimData::processRaw: setWritable --  h - %d\n",  h);
        }
    }
    stat.hoursInTotal = r4s->endHourInLog;
    return true;
}

hourStat_t *SkimData::getStat()
{
    return &stat;
}
        
void SkimData::process()
{
     uint8_t _hour = read();
     if (isToday())
         prepareFromMemory(); // take 1'st from memory
    
     prepareFromLog();    // 
     
     if (r4s) {
          processRaw();
          write();
     }  
}

//============================================================================================
// File System

void FileSystem::init() 
{
   // logDirName = "/log" ;
      
   // SPIFFS.begin(true);
   /*
    size_t totalBytes = 0, usedBytes = 0;
    totalBytes = SPIFFS.totalBytes();
    usedBytes  = SPIFFS.usedBytes();

    String temp_str = "totalBytes = " + String(totalBytes) + ", usedBytes = " +  String(usedBytes);
    println_w(temp_str);
*/
    char fname[32];
    
    buildDayFName(fname, sizeof(fname), logDirName, year(), month(), day());
   // scanLogDir(); //!!! log!!!
    printf_w("open file %s\n", fname);  //  "открыть файл не удалось"

    File root = SPIFFS.open(logDirName);
    if (!root)  SPIFFS.mkdir(logDirName);
    else        root.close();
    
    char *modifier = "w";
    if (SPIFFS.exists(fname))
        modifier = "a";
        
    if (!_canWrite)      
        return;
    _file = SPIFFS.open(fname, modifier);
    if (!_file) {
        printf_w("file open failed %s\n", fname);  //  "открыть файл не удалось"
    }
    if ("w" == modifier) {
        _file.print("V:1\n");
        
        rotateLogs();
    }
   // rotateLogs();//0000000000
}

int FileSystem::filenameDate(char *inputBuffer, int inputBuffer_size, int16_t year, int8_t month, int8_t  day) 
{
    return buildDayFName( inputBuffer, inputBuffer_size, logDirName,  year,  month, day);
}

void FileSystem::catFile(File f)
{
    while (f.available()) 
        write_w(f.read());
}

void catFile(File f)
{
    while (f.available()) 
        write_w(f.read());
}

void del_file(char *f_name)
{
    SPIFFS.remove( f_name ); 
}

bool str_to_tm (char **dataPointers, struct tm *tm_res ) 
{
    if (!dataPointers[REC_TIME_IDX])
         return false; 
    time_t _date = atoi(dataPointers[REC_TIME_IDX]);
    struct tm *tmstruct = localtime(&_date);    
    if (!tmstruct)
         return false;
    memcpy(tm_res, tmstruct, sizeof(struct tm));
    return true;
}

void FileSystem::listDir(fs::FS &fs, const char * dirname, bool need_cat, readFileHandler r_handler)
{
    File root = fs.open(dirname);
    if (!root) {
        printf_w("can't open dir '%s'\n", dirname);
        return;
    }
    if (!root.isDirectory()){
        printf_w("root is not dir '%s'\n", dirname);
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory())
            continue;
        else {
         //   time_t cr = 0;// file.getLastWrite();
         //   struct tm *tmstruct = localtime(&cr); 
            printf_w("  FILE: %s\tSIZE: %d\n", file.name(), file.size());
                        /*
            print_w(file.name());
            print_w("\tSIZE: ");
            println_w(file.size());
            file.getCreationTime();
            */
            if (need_cat) 
                 catFile(file);
            char *fname = strdup(file.name());
            file.close();

            if (r_handler)
                  r_handler(fname);
            
            free(fname);
        }
        file = root.openNextFile();
        
    }
}

bool  parseLine(char *_buffer, char **dataPointers, int dataPointersSize, char delimiter) 
{ // ahtung - spoil buffer!
      if (!_buffer)
            return false;
      bool nextIsNewData = true;
      int32_t _buffer_len = strlen(_buffer);
      int32_t field_cnt = 0;
      for (int i = 0; i < _buffer_len ; i++) {
            if (nextIsNewData) {
                if (field_cnt < dataPointersSize) 
                    dataPointers[field_cnt] = & _buffer[i];
                field_cnt++;
                nextIsNewData = false;
            }
            if (delimiter == _buffer[i]) {
                _buffer[i] = 0;
                nextIsNewData = true;
            } 
      }
      if (field_cnt != dataPointersSize)
            return false;
      return true;
} 

int aggregate_for_hour(char **dataPointers, int dataPointersSize, record_idx_t field_idx, float*temp_data, int8_t *temp_data_cnt)
{
    int current_hour = 0;
    if ( !dataPointers[REC_TIME_IDX] || 
          field_idx >= dataPointersSize ||
         !dataPointers[field_idx] )
              return -1; 

    time_t _date = atoi( dataPointers[REC_TIME_IDX] );
    struct tm *tmstruct = localtime(&_date); 
    temp_data[tmstruct->tm_hour] += atof( dataPointers[field_idx] );
    temp_data_cnt[tmstruct->tm_hour] += 1; 

    return tmstruct->tm_hour;
}  

void FileSystem::scanLogDir() 
{
 //   printf_w("start scanLogDir /\n");
 //   listDir(SPIFFS, "/");
    printf_w("start scanLogDir %s\n", logDirName);
    listDir(SPIFFS, logDirName, false, NULL);
    printf_w("start scanLogDir %s\n", skimDirName);
    listDir(SPIFFS, skimDirName, false, NULL);
   /*
    File fff = SPIFFS.open("/log/2021-01-17.txt");
    catFile(fff);
    fff.close();
    */
//    listDir(SPIFFS, skimDirName, false, del_file);
}

void FileSystem::saveRecordsToFile()
{
    if ( !_canWrite )
        return;

    statRecord_t *record = &RTC_RECORDS[0];
    char buf[MAX_LINE_SIZE];
    for (int i = 0; i < statRecord_cnt; i++, record++) {
        snprintf(buf, sizeof(buf), "%d\t%.2f\t%d\t%.2f\t%.2f\t%.2f\n", 
                record->Time, record->Vcc, record->Steps, record->HeartRate, record->AmbientTempC, record->ObjectTempC );
        printf_w(buf);
        _file.print(buf);
    }
}

void FileSystem::rotateLogs() 
{
    _rotateLogs( logDirName, filenamePattern, MAX_FILES_CNT);
    _rotateLogs( skimDirName, logFilenamePattern, 30);
}

void FileSystem::writeLog( statRecord_t *record )
{
  //  printf_w("RTC_RECORDS %p, statRecord_cnt %d\n", RTC_RECORDS, statRecord_cnt );
    //return ;
    if (record->Time < START_GOOD_DATE) 
        return;
    memcpy( (void*) &RTC_RECORDS[statRecord_cnt], (void*) record, sizeof(statRecord_t) );
    
    if (CACHE_RECORD_CNT == (statRecord_cnt + 1)){
        saveRecordsToFile();
        statRecord_cnt = 0;
    } else 
        statRecord_cnt++;
    
}

void FileSystem::close()
{
    if ( !_canWrite )
        return;
    _file.close();
}
