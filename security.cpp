#include "security.h"
#include <map>
#include <iostream>   // std::cout
#include <string>     // std::string, std::stol
#include <cstdlib>
#include <algorithm>    // std::sort
#include <vector>       // std::vector
#include <algorithm>
#include <locale>

#include "sqlite3.h"
#include "Eigen/Core"
#include "Eigen/Dense"
#include "logger.hpp"
#include "timeseries.hpp"
#include "yahoofetch.hpp"
#include "googlefetch.hpp"
#include "netutils.hpp"
#include "md5.h"
#include "helpers.hpp"
using namespace std;
    vector< vector<string> >  security::querydb(string dbname,string selectq)
    {
        sqlite3 *db;
        sqlite3_stmt * statement;
        vector< vector<string> > res;
        /* Open database */
        if (sqlite3_open(dbname.c_str(), &db)!=SQLITE_OK)
            THROW_EXCEPTION("Cannot open database "<<dbname);

        /* Create SQL statement */


        /* Execute SQL statement */
        if(sqlite3_prepare_v2(db, (selectq +";").c_str(), -1, &statement, 0) == SQLITE_OK)
        {
            int cols = sqlite3_column_count(statement);
            int result = 0;
            while(true)
            {
                result = sqlite3_step(statement);

                if(result == SQLITE_ROW)
                {
                    vector<string> t1;
                    for(int col = 0; col < cols; col++)
                    {
                        string s = sqlite3_column_text(statement, col)==NULL?"":(char*)sqlite3_column_text(statement, col);
                        t1.push_back(s);
                        //do something with it
                    }
                    res.push_back(t1);
                }
                else
                {
                    break;
                }

            }

            sqlite3_finalize(statement);

        }


        sqlite3_close(db);
        return res;
    }


    //values=name,freq,date,o,h,l,c,v,oi
     const vector< vector<string> > security::readCSV(string filename,unsigned tokens,char delimiter)
    {
        ifstream myfile( filename.c_str() );
        if (!myfile) throw std::invalid_argument("cannot open file "+ filename);
        vector< vector<string> > result;
        unsigned it=0;
        while (!myfile.eof())
        {
            string line;
            it++;
            vector<string> tmp;
            size_t pos=string::npos;
            getline(myfile,line);
            while ((pos=line.find_first_of(delimiter))!=string::npos)
            {
                tmp.push_back(line.substr(0,pos));
                line.erase(0,pos+1);
            }
            if (line.size()>0) tmp.push_back(line);
            if (tmp.size()==tokens) result.push_back(tmp);
        }
        myfile.close();
        return result;
    }
//aggiusta i valori forniti dalla routine 'quotes' della libreria 'yahoofetch' in modo da avere un vettore di vettori compatibile con il popolamento db
    const vector< vector<string> > YahooAdjust(string symbol, string freq,string market,const vector<vector<string> > & quotes)
    {
        vector< vector<string> > ret;
        vector< vector<string> >::const_iterator it;

        for (auto &v  : quotes)
        {
            if (v.size()!=7) THROW_EXCEPTION("size must be 7");
            if (v.at(0).size()!=10) THROW_EXCEPTION("size must be 10 YYYY-MM-DD:"<<v.at(0));
            vector<string> nv;
            string YYYY=v.at(0).substr(0,4);
            string MM=v.at(0).substr(5,2);
            string DD=v.at(0).substr(8,2);
            string date=YYYY+MM+DD;
            double adj=std::stod(v.at(6));
            double close=std::stod(v.at(4));
            double factor=adj/close;
            double open=std::stod(v.at(1))*factor;
            double high=std::stod(v.at(2))*factor;
            double low=std::stod(v.at(3))*factor;
            close=std::stod(v.at(4))*factor;
            std::to_string(close);
            nv.push_back(symbol);
            nv.push_back(market);
            nv.push_back(freq);

            nv.push_back(date);
            nv.push_back(std::to_string(open));
            nv.push_back(std::to_string(high));
            nv.push_back(std::to_string(low));
            nv.push_back(std::to_string(close));
            nv.push_back(v.at(5));
            nv.push_back(std::to_string(0.0));//open interest
            ret.push_back(nv);
        }



        return ret;
    }
     void security::createDB(string name)
    {
        string sql;
        sqlite3 *db;
        char *zErrMsg = 0;
        int rc;

        rc = sqlite3_open(name.c_str(), &db);

        if( rc )
        {
            THROW_EXCEPTION("cannot create db "<<name);
        }
        else
        {

            /* Create SQL statement */
            sql = "CREATE TABLE if not exists SECURITIES("  \
                  "NAME           TEXT    NOT NULL," \
                  "MARKET         TEXT    NOT NULL,"\
                  "FREQ            TEXT NOT NULL," \
                  "DATE            INT NOT NULL," \
                  "OPEN            REAL NOT NULL," \
                  "HIGH            REAL NOT NULL," \
                  "LOW            REAL NOT NULL," \
                  "CLOSE            REAL NOT NULL," \
                  "VOLUME            REAL NOT NULL," \
                  "OPEN_INTEREST         REAL NOT NULL,"\
                  "PRIMARY KEY (NAME,FREQ,DATE));";

            /* Execute SQL statement */

            rc = sqlite3_exec(db, sql.c_str(), 0, 0, &zErrMsg);
            if( rc != SQLITE_OK )
            {
                string es(zErrMsg);
                sqlite3_free(zErrMsg);
                sqlite3_close(db);
                THROW_EXCEPTION("cannot execute query "<<sql<<"\n"<<es);

            }

            /* Create SQL statement */
            sql = "CREATE TABLE if not exists SECINFO("  \
                  "ISIN           TEXT    NOT NULL," \
                  "NAME            TEXT NOT NULL," \
                  "CODE            TEXT NOT NULL," \
                  "SECTOR            TEXT ," \
                  "CURRENCY         TEXT NOT NULL," \
                  "MARKET         TEXT NOT NULL," \
                  "TYPE         TEXT NOT NULL," \
                  "YAHOOSYM     TEXT ," \
                  "GOOGLESYM         TEXT ," \
                  "PRIMARY KEY (ISIN));";

            /* Execute SQL statement */

            rc = sqlite3_exec(db, sql.c_str(), 0, 0, &zErrMsg);
            if( rc != SQLITE_OK )
            {
                string es(zErrMsg);
                sqlite3_free(zErrMsg);
                sqlite3_close(db);
                THROW_EXCEPTION("cannot execute query "<<sql<<"\n"<<es);

            }


            sqlite3_close(db);

        }

    }

    void security::insert_db_SECINFO(string dbname,string isin,string name, string code,string sector,string currency,string market,string type, string ysym,string gsym) {
        sqlite3 *db;
        int rc;
        sqlite3_stmt *statement;
        const char *query = "INSERT OR REPLACE INTO SECINFO VALUES(?1,?2,?3,?4,?5,?6,?7,?8,?9);";
        rc = sqlite3_open(dbname.c_str(), &db);
        if( rc )   THROW_EXCEPTION("cannot open database "<< dbname<<" : "<<sqlite3_errmsg(db));
        if (sqlite3_prepare_v2(db,query,-1,&statement,NULL))
        {
            const char* c =sqlite3_errmsg(db);
            string  es(c);
            sqlite3_close(db);
            THROW_EXCEPTION("cannot create statement:"<<es);
        }
        sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);

        map<string, map<string,string> >::const_iterator it;

        try
        {

                if (sqlite3_bind_text(statement,1,isin.c_str(),isin.size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,2,name.c_str(),name.size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,3,code.c_str(),code.size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,4,sector.c_str(),sector.size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,5,currency.c_str(),currency.size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,6,market.c_str(),market.size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,7,type.c_str(),type.size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,8,ysym.c_str(),ysym.size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,9,gsym.c_str(),gsym.size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_step(statement)!=SQLITE_DONE)
                {
                    const char* c =sqlite3_errmsg(db);
                    string  es(c);
                    THROW_EXCEPTION("statement step error: "<<es);
                }
                sqlite3_reset(statement);
                //  cout <<it2->first<<"\t"<<it2->second<<"\n";

            sqlite3_exec(db, "COMMIT TRANSACTION", 0, 0, 0);

        }
        catch (std::exception &e)
        {

            std::stringstream s1;
            s1 <<"rollback status="<<sqlite3_exec(db, "ROLLBACK TRANSACTION", 0, 0, 0);
            LOG_WARN(s1);
            string w=e.what();
            if (statement!=NULL) sqlite3_finalize(statement);
            if (db!=NULL) sqlite3_close(db);
            THROW_EXCEPTION("error inserting values "<<w);
        }
        if (statement!=NULL) sqlite3_finalize(statement);
        if (db!=NULL) sqlite3_close(db);

    }


const map< string,map<string,string> > fetchStock();
const map< string,map<string,string> > fetchETF();
const map< string,map<string,string> > fetchUSAStock();



void populate_special_SECINFO(string db) {
    map<string,string> m;
    m["S&P 500"]="^GSPC";
    m["NASDAQ Composite"]="^IXIC";
    m["Dow Jones Industrial Average"]="^DJI";
    for (auto &v: m)    security::insert_db_SECINFO(db,md5(v.first),v.first,v.second,"","USD","","INDEX",v.second,"");
    m.clear();
    m["FTSE 100"]="^FTSE";
    for (auto &v: m)    security::insert_db_SECINFO(db,md5(v.first),v.first,v.second,"","GBP","","INDEX",v.second,"");
    m.clear();
    m["DAX"]="^GDAXI";
    m["CAC 40"]="^FCHI";
    m["FTSEMIB"]="FTSEMIB.MI";
    for (auto &v: m)    security::insert_db_SECINFO(db,md5(v.first),v.first,v.second,"","EUR","","INDEX",v.second,"");
    m.clear();
    m["Nikkei 225"]="^N225";
    for (auto &v: m)    security::insert_db_SECINFO(db,md5(v.first),v.first,v.second,"","JPY","","INDEX",v.second,"");
    m.clear();
    m["HANG SENG INDEX"]="^HSI";
    for (auto &v: m)    security::insert_db_SECINFO(db,md5(v.first),v.first,v.second,"","HKD","","INDEX",v.second,"");
    m.clear();
    m["SSE Composite Index"]="000001.SS";
    for (auto &v: m)    security::insert_db_SECINFO(db,md5(v.first),v.first,v.second,"","CNY","","INDEX",v.second,"");
    m.clear();
    m["SMI"]="^SSMI";
    for (auto &v: m)    security::insert_db_SECINFO(db,md5(v.first),v.first,v.second,"","CHF","","INDEX",v.second,"");
    m.clear();
    m["ALL ORDINARIES"]="^AORD";
    for (auto &v: m)    security::insert_db_SECINFO(db,md5(v.first),v.first,v.second,"","AUD","","INDEX",v.second,"");
    m.clear();
    m["VOLATILITY S&P 500"]="^VIX";
    for (auto &v: m)    security::insert_db_SECINFO(db,md5(v.first),v.first,v.second,"","","","INDEX",v.second,"");
    m.clear();



}
     void security::populate_db_SECINFO(string dbname)
    {

         populate_special_SECINFO(dbname);
        map<string, map<string,string> > m=fetchStock();//mercato italiano
        map<string, map<string,string> > m2=fetchETF();//mercato italiano
        map<string, map<string,string> > m3=fetchUSAStock();//mercato americano (NASDAQ,NYSE,AMEX)
        sqlite3 *db;
        int rc;
        sqlite3_stmt *statement;
        const char *query = "INSERT OR REPLACE INTO SECINFO VALUES(?1,?2,?3,?4,?5,?6,?7,?8,?9);";
        rc = sqlite3_open(dbname.c_str(), &db);
        if( rc )   THROW_EXCEPTION("cannot open database "<< dbname<<" : "<<sqlite3_errmsg(db));
        if (sqlite3_prepare_v2(db,query,-1,&statement,NULL))
        {
            const char* c =sqlite3_errmsg(db);
            string  es(c);
            sqlite3_close(db);
            THROW_EXCEPTION("cannot create statement:"<<es);
        }
        sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);

        map<string, map<string,string> >::const_iterator it;

        try
        {
            for (it=m.begin(); it!=m.end(); it++)
            {


                if (sqlite3_bind_text(statement,1,it->second.at("ISIN").c_str(),it->second.at("ISIN").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,2,it->second.at("NAME").c_str(),it->second.at("NAME").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,3,it->second.at("CODE").c_str(),it->second.at("CODE").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,4,it->second.at("SECTOR").c_str(),it->second.at("SECTOR").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,5,"EUR",string("EUR").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,6,"MLSE",string("MLSE").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,7,"STOCK",string("STOCK").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                sqlite3_bind_null(statement, 8);
                string codey="BIT:"+it->second.at("CODE");
                if (sqlite3_bind_text(statement,9,codey.c_str(),codey.size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_step(statement)!=SQLITE_DONE)
                {
                    const char* c =sqlite3_errmsg(db);
                    string  es(c);
                    THROW_EXCEPTION("statement step error: "<<es);
                }
                sqlite3_reset(statement);
                //  cout <<it2->first<<"\t"<<it2->second<<"\n";
            }
            for (it=m2.begin(); it!=m2.end(); it++)
            {


                if (sqlite3_bind_text(statement,1,it->second.at("ISIN").c_str(),it->second.at("ISIN").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,2,it->second.at("NAME").c_str(),it->second.at("NAME").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,3,it->second.at("CODE").c_str(),it->second.at("CODE").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,4,it->second.at("SECTOR").c_str(),it->second.at("SECTOR").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,5,"EUR",string("EUR").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,6,"MLSE",string("MLSE").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,7,"ETF",string("ETF").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                string codey=it->second.at("CODE")+".MI";
                if (sqlite3_bind_text(statement,8,codey.c_str(),codey.size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_null(statement, 9))  THROW_EXCEPTION("binding error");

                if (sqlite3_step(statement)!=SQLITE_DONE)
                {
                    const char* c =sqlite3_errmsg(db);
                    string  es(c);
                    THROW_EXCEPTION("statement step error: "<<es);
                }
                sqlite3_reset(statement);
                //  cout <<it2->first<<"\t"<<it2->second<<"\n";
            }


            for (it=m3.begin(); it!=m3.end(); it++)
            {


                if (sqlite3_bind_text(statement,1,it->second.at("ISIN").c_str(),it->second.at("ISIN").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,2,it->second.at("NAME").c_str(),it->second.at("NAME").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,3,it->second.at("CODE").c_str(),it->second.at("CODE").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,4,it->second.at("SECTOR").c_str(),it->second.at("SECTOR").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,5,"USD",string("USD").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,6,"NYSE",string("NYSE").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_text(statement,7,"STOCK",string("STOCK").size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                string codey=it->second.at("CODE");
                if (sqlite3_bind_text(statement,8,codey.c_str(),codey.size(),SQLITE_TRANSIENT )) THROW_EXCEPTION("binding error") ;
                if (sqlite3_bind_null(statement, 9))  THROW_EXCEPTION("binding error");

                if (sqlite3_step(statement)!=SQLITE_DONE)
                {
                    const char* c =sqlite3_errmsg(db);
                    string  es(c);
                    THROW_EXCEPTION("statement step error: "<<es);
                }
                sqlite3_reset(statement);
                //  cout <<it2->first<<"\t"<<it2->second<<"\n";
            }



            sqlite3_exec(db, "COMMIT TRANSACTION", 0, 0, 0);

        }
        catch (std::exception &e)
        {

            std::stringstream s1;
            s1 <<"rollback status="<<sqlite3_exec(db, "ROLLBACK TRANSACTION", 0, 0, 0);
            LOG_WARN(s1);
            string w=e.what();
            if (statement!=NULL) sqlite3_finalize(statement);
            if (db!=NULL) sqlite3_close(db);
            THROW_EXCEPTION("error inserting values "<<w);
        }
        if (statement!=NULL) sqlite3_finalize(statement);
        if (db!=NULL) sqlite3_close(db);

    }

     void security::populate_db(string dbname,const vector< vector<string> >& values)
    {
        sqlite3 *db;
        int rc;
        sqlite3_stmt *statement;
        struct tm *tt= new tm;//localtime(&t1);
        const char *query = "INSERT OR REPLACE INTO SECURITIES VALUES(?1,?2,?3,?4,?5,?6,?7,?8,?9,?10);";
        rc = sqlite3_open(dbname.c_str(), &db);
        if( rc )   THROW_EXCEPTION("cannot open database "<< dbname<<" : "<<sqlite3_errmsg(db));
        if (sqlite3_prepare_v2(db,query,-1,&statement,NULL))
        {
            const char* c =sqlite3_errmsg(db);
            string  es(c);
            sqlite3_close(db);
            THROW_EXCEPTION("cannot create statement:"<<es);
        }
        sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);
        //unsigned pcent=0,total=values.size();
        //double th=0.0;

        vector< vector<string> >::const_iterator it;
        try
        {
            for (it=values.begin(); it!=values.end(); it++)
            {
              //  pcent++;
               // double t1=((double)pcent/(double)total);
               // if (t1>th)
               // {

                 //   LOG_DEBUG("load progress "<<ceil(t1*100)<<"%");
                   // th=th+0.01;

                //}
                if ((*it).size()!=10) continue;
                string name=it->at(0);
                string market=it->at(1);
                string freq=it->at(2);
                //unsigned date=std::strtol (it->at(2).c_str(),NULL,10);
                unsigned date=std::strtol (it->at(3).c_str(),NULL,10);//YYYYMMDD
                unsigned day_t=date % 100;
                unsigned month_t=((date % 10000)-day_t)/100;
                unsigned year_t=(date-month_t-day_t)/10000 ;
                tt->tm_mday=day_t;
                tt->tm_year=year_t-1900;
                tt->tm_mon=month_t-1;
                tt->tm_isdst=-1;
                tt->tm_hour=0;
                tt->tm_min=0;
                tt->tm_sec=0;
                time_t date_timet=mktime(tt);


                double open=std::strtod (it->at(4).c_str(),NULL);
                double high=std::strtod (it->at(5).c_str(),NULL);
                double low=std::strtod (it->at(6).c_str(),NULL);
                double close=std::strtod (it->at(7).c_str(),NULL);
                double volume=std::strtod (it->at(8).c_str(),NULL);
                double oi=std::strtod (it->at(9).c_str(),NULL);

                if (sqlite3_bind_text(statement,1,name.c_str(),name.size(),SQLITE_TRANSIENT )) throw std::runtime_error("binding error") ;
                if (sqlite3_bind_text(statement,2,market.c_str(),market.size(),SQLITE_TRANSIENT )) throw std::runtime_error("binding error") ;
                if (sqlite3_bind_text(statement,3,freq.c_str(),freq.size(),SQLITE_TRANSIENT )) throw std::runtime_error("binding error") ;
                if (sqlite3_bind_int (statement,4,date_timet)) throw std::runtime_error("binding error") ;
                if (sqlite3_bind_double(statement,5,open)) throw std::runtime_error("binding error") ;
                if (sqlite3_bind_double(statement,6,high)) throw std::runtime_error("binding error") ;
                if (sqlite3_bind_double(statement,7,low)) throw std::runtime_error("binding error") ;
                if (sqlite3_bind_double(statement,8,close)) throw std::runtime_error("binding error") ;
                if (sqlite3_bind_double(statement,9,volume)) throw std::runtime_error("binding error") ;
                if (sqlite3_bind_double(statement,10,oi)) throw std::runtime_error("binding error") ;
                if (sqlite3_step(statement)!=SQLITE_DONE)
                {
                    const char* c =sqlite3_errmsg(db);
                    string  es(c);
                    THROW_EXCEPTION("statement step error: "<<es);
                }
                sqlite3_reset(statement);
            }
            sqlite3_exec(db, "COMMIT TRANSACTION", 0, 0, 0);
        }
        catch (std::exception &e)
        {

            std::stringstream s1;
            s1 <<"rollback status="<<sqlite3_exec(db, "ROLLBACK TRANSACTION", 0, 0, 0);
            LOG_WARN(s1);
            string w=e.what();
            if (statement!=NULL) sqlite3_finalize(statement);
            if (tt) delete tt;
            if (db!=NULL) sqlite3_close(db);

            THROW_EXCEPTION("error inserting values "<<w);
        }
        if (statement!=NULL) sqlite3_finalize(statement);
        if (db!=NULL) sqlite3_close(db);
        if (tt) delete tt;

    }
     const map<string,string> security::getSecFreqList(string dbname)
    {
        map<string,string> ret;
        sqlite3 *db;
        sqlite3_stmt *statement;
        int rc;
        string query = "SELECT distinct NAME,FREQ FROM SECURITIES  order by NAME asc;";
        rc = sqlite3_open(dbname.c_str(), &db);
        if( rc )   THROW_EXCEPTION("cannot open database "<< dbname<<" : "<<sqlite3_errmsg(db));
        if (sqlite3_prepare_v2(db,query.c_str(),-1,&statement,NULL))
        {
            const char* c =sqlite3_errmsg(db);
            string  es(c);
            sqlite3_close(db);
            THROW_EXCEPTION("cannot create statement:"<<es);
        }
        do
        {
            rc = sqlite3_step (statement ) ;
            if (rc == SQLITE_ROW)   /* can read data */
            {
                string nm = (const char*)(sqlite3_column_text(statement,0));
                string fq = (const char*)sqlite3_column_text(statement,1);
                std::pair<string,string> p;
                p.first=nm;
                p.second=fq;
                ret.insert(p);
                //ret.push_back(nm+"-"+fq);
            }

        }
        while (rc == SQLITE_ROW);

        sqlite3_finalize(statement);
        sqlite3_close(db) ;
        return ret;

    }

     const vector<string> security::getSecList(string dbname,string sector,string currency,string market,string type)
    {
        vector<string> ret;
        sqlite3 *db;
        sqlite3_stmt *statement;
        int rc;
        string qadd="";
        string query = "SELECT distinct CODE FROM SECINFO  ";//order by NAME asc;";
        if (sector!="") qadd+=" sector like '%"+sector+"%' ";
        if (currency!="") qadd+=qadd==""?" currency='"+currency+"' ":" and currency='"+currency+"' ";
        if (market!="") qadd+=qadd==""?" market='"+market+"' ":" and market='"+market+"' ";
        if (type!="") qadd+=qadd==""?" type='"+type+"' ":" and type='"+type+"' ";
        if (qadd!="") qadd=" where "+qadd+" order by NAME asc;";else qadd=" order by NAME asc;";
        query+=qadd;
        rc = sqlite3_open(dbname.c_str(), &db);
        if( rc )   THROW_EXCEPTION("cannot open database "<< dbname<<" : "<<sqlite3_errmsg(db));
        if (sqlite3_prepare_v2(db,query.c_str(),-1,&statement,NULL))
        {
            const char* c =sqlite3_errmsg(db);
            string  es(c);
            sqlite3_close(db);
            THROW_EXCEPTION("cannot create statement:"<<es);
        }
        do
        {
            rc = sqlite3_step (statement ) ;
            if (rc == SQLITE_ROW)   /* can read data */
            {
                string nm = (const char*)(sqlite3_column_text(statement,0));
                //string fq = (const char*)sqlite3_column_text(statement,1);
                ret.push_back(nm);
            }

        }
        while (rc == SQLITE_ROW);

        sqlite3_finalize(statement);
        sqlite3_close(db) ;
        return ret;

    }


void security::loadDailyQuotesYahoo(string db) {
    vector<vector<string> > vv=security::querydb(db,"select CODE,MARKET,YAHOOSYM from SECINFO where YAHOOSYM is not null and YAHOOSYM<>\"\"");
    long tot=vv.size(),cnt=1;

    for (auto &z: vv)
    {
            LOG_INFO("loading "<<z[0]<<"\t"<<z[1]<<"\tyahoo:"<<z[2]<<"\t"<<cnt<<" of "<<tot);
            try
            {
                vector< vector<string> > ya=YahooAdjust(z[0],"D",z[1], yahoofetch::quotes(z[2]));
                if (ya.size()>0)
                    security::populate_db(db,ya);
                    else LOG_DEBUG("no data for  "<<z[0]);
            }
            catch (std::exception &e)
            {
                LOG_ERROR(e.what());
            }
            cnt++;

    }

}

void security::loadDailyQuotesGoogle(string db) {
    vector<vector<string> > vv=security::querydb(db,"select CODE,MARKET,GOOGLESYM from SECINFO where GOOGLESYM is not null and GOOGLESYM<>\"\"");
    long tot=vv.size(),cnt=1;
    for (auto &z: vv)
    {
            LOG_INFO("loading "<<z[0]<<"\t"<<z[1]<<"\tgoogle:"<<z[2]<<"\t"<<cnt<<" of "<<tot);
            try
            {
                //vector< vector<string> > ya=security::YahooAdjust(z[0],"D",z[1], yahoofetch::quotes(z[0]));
                vector< vector<string> > ya=googlefetch::googlefetch(z[0],z[1]);

                if (ya.size()>0)
                    security::populate_db(db,ya);
                    else LOG_DEBUG("no data for  "<<z[0]);
            }
            catch (std::exception &e)
            {
                LOG_ERROR(e.what());
            }
            cnt++;
    }

}



const timeseries security::init(string dbname,string symbol,string market,timeseries::freq_enum f){

    Eigen::MatrixXd M;
    std::vector<time_t> dates;
    //string  freq;
        //this->name=symbol;
      //  freq=freq;
        std::vector<std::string> tsnames;
        sqlite3 *db;
        sqlite3_stmt *statcnt;
        sqlite3_stmt *statement;
try {

        tsnames.push_back("open("+symbol+")");
        tsnames.push_back("high("+symbol+")");
        tsnames.push_back("low("+symbol+")");
        tsnames.push_back("close("+symbol+")");
        tsnames.push_back("volume("+symbol+")");
        tsnames.push_back("oi("+symbol+")");

        int rc;
        unsigned len;
        string freq;
        if (f==timeseries::freq_enum::daily) freq="D";
        else if (f==timeseries::freq_enum::weekly) freq="W";
        else if (f==timeseries::freq_enum::monthly) freq="M";
        else THROW_EXCEPTION("frequence not found :"<<f);

        string querycnt = "SELECT count(*)  FROM SECURITIES WHERE  NAME='"+symbol+"' and MARKET='"+market+"' and freq='"+freq+"';";
        string query = "SELECT *  FROM SECURITIES WHERE NAME='"+symbol+"' and MARKET='"+market+"' and freq='"+freq+"' order by date asc;";
        rc = sqlite3_open(dbname.c_str(), &db);
        if( rc )   THROW_EXCEPTION("cannot open database "<< dbname<<" : "<<sqlite3_errmsg(db));

        if (sqlite3_prepare_v2(db,querycnt.c_str(),-1,&statcnt,NULL))
        {
            const char* c =sqlite3_errmsg(db);
            string  es(c);
            sqlite3_close(db);
            THROW_EXCEPTION("cannot create statement:"<<es);
        }

        rc = sqlite3_step (statcnt ) ;
        if (rc == SQLITE_ROW)
        {
            len=sqlite3_column_int(statcnt,0);
            sqlite3_finalize(statcnt);
        }
        else
        {
            sqlite3_finalize(statcnt);

            sqlite3_close(db);
            THROW_EXCEPTION("error fetching security "<<this->name);
        }

        if (len==0) THROW_EXCEPTION("no data for symbol "<<this->name);
        M.resize(len,6);//open,high,low,close,volume,oi

        if (sqlite3_prepare_v2(db,query.c_str(),-1,&statement,NULL))
        {
            const char* c =sqlite3_errmsg(db);
            string  es(c);
            sqlite3_close(db);
            THROW_EXCEPTION("cannot create statement:"<<es);
        }
        unsigned k=0;
        dates.clear();
        do
        {
            rc = sqlite3_step (statement ) ;
            if (rc == SQLITE_ROW)   /* can read data */
            {
                time_t date = sqlite3_column_int(statement,3) ;
                double open = sqlite3_column_double(statement,4) ;
                double high = sqlite3_column_double(statement,5) ;
                double low = sqlite3_column_double(statement,6) ;
                double close = sqlite3_column_double(statement,7) ;
                double volume = sqlite3_column_double(statement,8) ;
                double oi = sqlite3_column_double(statement,9);
                dates.push_back(date);

                M(k,0)=open;
                M(k,1)=high;
                M(k,2)=low;
                M(k,3)=close;
                M(k,4)=volume;
                M(k,5)=oi;

                k++;
            }

        }
        while (rc == SQLITE_ROW);

        sqlite3_finalize(statement);
        sqlite3_close(db) ;
        return timeseries(tsnames,dates,f,M);
        } catch (std::exception &e) {
          // if (statement) sqlite3_finalize(statement);
            if (db) sqlite3_close(db) ;
            THROW_EXCEPTION(string(e.what()))
        }

}


string trimstr(const string & str) {
    if (str.size()<1) return str;
    string test=str;
    const string whitespace = " \t\f\v\n\r";
    int start = test.find_first_not_of(whitespace);
    int end = test.find_last_not_of(whitespace);
    test.erase(0,start);
    test.erase((end - start) + 1);
    return test;
}
vector<string> split(string str, char delimiter)
{
    vector<string> internal;
    stringstream ss(str); // Turn the string into a stream.
    string tok;

    while(getline(ss, tok, delimiter))
    {
        internal.push_back(tok);
    }
    return internal;
}


const map< string,map<string,string> > fetchUSAStock()
{

    /*
            case 'A': return 'AMEX';
            case 'N': return 'NYSE';
            case 'P': return 'NYSEArca';
            case 'S': return 'NASDAQ-CM';
            case 'Q': return 'NASDAQ-GS';
            case 'G': return 'NASDAQ-GM';
            case 'W': return 'CBOE';
            case 'Z': return 'BATS';
            default: return 'Undef';
      */
    map< string,map<string,string> > risultato;
    map<string,string> row;

    //Symbol|Security Name|Market Category|Test Issue|Financial Status|Round Lot Size
    string url_nasdaq=string("ftp://ftp.nasdaqtrader.com/SymbolDirectory/nasdaqlisted.txt");
    //ACT Symbol|Security Name|Exchange|CQS Symbol|ETF|Round Lot Size|Test Issue|NASDAQ Symbol
    string url_others=string("ftp://ftp.nasdaqtrader.com/SymbolDirectory/otherlisted.txt");
    try
    {
        string nasdaq= netutils::httpfetch(url_nasdaq);
        string others= netutils::httpfetch(url_others);
        vector<string> nsplit=split(nasdaq,'\n');
        vector<string> osplit=split(others,'\n');
        for (string s : nsplit)
        {
            vector<string> nsplit2=split(s,'|');
            if (nsplit2.size()!=6) continue;
            bool ok=true;
            for (size_t k=0; k<6; k++)
            {
                if (nsplit2.at(k).size()<1) ok=false;
                if (k==1)
                    if (nsplit2.at(k).find("Common")== std::string::npos || nsplit2.at(k).find("Stock")== std::string::npos) ok=false;
            }
            if (ok)
            {

                //cout<<"\n"<<s;
                row.clear();
                row["ISIN"]=md5(trimstr(nsplit2.at(1)));
                row["NAME"]=trimstr(nsplit2.at(1));
                row["CODE"]=trimstr(nsplit2.at(0));
                row["SECTOR"]="UNKNOW";
                risultato[md5(nsplit2.at(1))]=row;
            }
        }
        for (string s : osplit)
        {
            vector<string> nsplit2=split(s,'|');
            if (nsplit2.size()!=8) continue;
            bool ok=true;
            for (size_t k=0; k<8; k++)
            {
                if (nsplit2.at(k).size()<1) ok=false;
                if (k==1)
                    if (nsplit2.at(k).find("Common")== std::string::npos || nsplit2.at(k).find("Stock")== std::string::npos) ok=false;
                if (k==2)
                    if (!(nsplit2.at(k) =="A" || nsplit2.at(k)=="N"  )) ok=false;
            }
            if (ok)
            {
               // cout<<"\n"<<s;
                row.clear();
                row["ISIN"]=md5(trimstr(nsplit2.at(1)));
                row["NAME"]=trimstr(nsplit2.at(1));
                row["CODE"]=trimstr(nsplit2.at(0));
                row["SECTOR"]="UNKNOW";
                risultato[md5(nsplit2.at(1))]=row;

            }
        }

    }



    catch (std::exception &e)
    {
        THROW_EXCEPTION(e.what());
    }
    return risultato;

}


const map< string,map<string,string> > fetchStock() {
    map< string,map<string,string> > risultato;

    int np=1;
    char letter='A';
    std::string start_str="list-aZ-stream";
    std::string fs="/scheda/";
    std::string mta;
    do {
        bool nuovalettera=false;
        do {
            string url=string("http://www.borsaitaliana.it/borsa/azioni/listino-a-z.html?initial=")+std::string(1,letter) + string("&page=") + std::to_string(np) + string("&lang=en");
            LOG_DEBUG(url);
            mta = netutils::httpfetch(url);
            unsigned long k1 = 0, k2=0;
            
            if (mta.find("title=\"Next\"", 0)==string::npos)  nuovalettera=true;
            while (true) {

                    k2 =  mta.find(fs, k1);
                    if (k2==string::npos) break;
                    map<string,string> _map;
                    string t1 =  mta.substr(k2+fs.size(),12);//ISIN
                    string t2;
                    LOG_DEBUG("ISIN "<<t1);
                    string isin=t1;
                    ///borsa/azioni/scheda/IT0001233417.html?lang=en
                    //string fn = string("/scheda.html?isin=") + t1 ;//+ "&lang=en\">";
                    string fn = string("/scheda/")+t1+string(".html?lang=en");
                    int k3 =   mta.find(fn);
                    k3=mta.find("<strong>",k3);
                    int k4 = mta.find("</strong>", k3);
                    string nome;
                    if (k3>0 && k4>0) {
                        t2 = mta.substr((k3 +8), k4-k3-8);
                        nome=helpers::Trim(t2);
                        LOG_DEBUG("NOME "<<nome);
                    } else LOG_DEBUG("NOME VUOTO");
                    //string url2=string("http://www.borsaitaliana.it/borsa/azioni/scheda.html?isin=")+t1+string("&lang=en");
                    string url2=string("http://www.borsaitaliana.it/borsa/azioni/scheda/") +t1+string(".html?lang=en");
                    LOG_DEBUG(url2);
                    string detail=netutils::httpfetch(url2);
                    int k5=detail.find("Alphanumeric Code");
                    k5=detail.find("<span class=\"t-text -right\">", k5);
                    int k6=detail.find("</span>", k5);
                    string alpha;
                    if (k5>0 && k6>0) {
                        alpha=helpers::Trim(detail.substr(k5+28, k6-k5-28));
                        LOG_DEBUG("ALFANUMERICO "<<alpha);
                    } else LOG_DEBUG("ALFANUMERICO VUOTO");
                    k5=detail.find("Super Sector");
                    k5=detail.find("<span class=\"t-text -right\">", k5);
                    k6=detail.find("</span>", k5);
                    string sector;
                    if (k5>0 && k6>0) {
                        sector=helpers::Trim(detail.substr(k5+28, k6-k5-28));
                        LOG_DEBUG("SETTORE "<< sector);
                    } else LOG_DEBUG("SETTORE VUOTO");
                    if (risultato.find(isin)==risultato.end() ) {
                        _map["ISIN"]=trimstr(isin);
                        _map["NAME"]=trimstr(nome);
                        _map["CODE"]=trimstr(alpha);
                        _map["SECTOR"]=trimstr(sector);
                        risultato[isin]=_map;
                        LOG_DEBUG("added "<<isin <<"  "<<nome);
                    } 
                    k1 = k2+fs.size();

                }

                np++;
            }while (!nuovalettera);
            letter++;
            np=1;
        } while (letter <= 'Z');
        return risultato;
    }


const map< string,map<string,string> > fetchETF() {
    map< string,map<string,string> > risultato;

    int np=1;
    //char letter='A';

    string mta;

        bool ultimapagina=false;
        do {
            string url=string("http://www.borsaitaliana.it/borsa/etf.html?&page=")+ std::to_string(np) + string("&lang=en");
            LOG_DEBUG(url);
            mta = netutils::httpfetch(url);
            unsigned long k1 = 0, k2;
            if (mta.find("title=\"Next\"", k1)==string::npos)  ultimapagina=true;

            //<a href="/borsa/etf/scheda.html?isin=FR0007080973&amp;lang=en" title="&nbsp;Amundi Cac 40 Ucits Etf">Amundi Cac 40 Ucits Etf</a>
            while ((k2 =  mta.find("/borsa/etf/scheda/", k1) ) != string::npos) {
                    map<string,string> _map;
                    string t1 =  mta.substr(k2+18,12);//ISIN
                    string t2;
                    LOG_DEBUG("ISIN "<<t1);
                    string isin=t1;
                    //string fn = string("/borsa/etf/scheda.html?isin=") + t1 ;//+ "&lang=en\">";
                    string fn = string("/scheda/")+t1+string(".html?lang=en");
                    int k3 =   mta.find(fn);
                    k3=mta.find(">",k3);
                    int k4 = mta.find("</a>", k3);
                    string nome;
                    if (k3>0 && k4>0) {
                        t2 = mta.substr((k3 +1), k4-k3-1);
                        nome=helpers::Trim(t2);
                        LOG_DEBUG("NOME "<<nome);
                    } else LOG_DEBUG("NOME VUOTO");
                    //string url2=string("http://www.borsaitaliana.it/borsa/etf/scheda.html?isin=")+t1+string("&lang=en");
                    string url2=string("http://www.borsaitaliana.it/borsa/etf/scheda/") +t1+string(".html?lang=en");
                    LOG_DEBUG(url2);
                    string detail=netutils::httpfetch(url2);
                    int k5=detail.find("Alphanumeric Code");
                    k5=detail.find("<span class=\"t-text -right\">", k5);
                    int k6=detail.find("</span>", k5);
                    string alpha;
                    if (k5>0 && k6>0) {
                        alpha=helpers::Trim(detail.substr(k5+28, k6-k5-28));
                        LOG_DEBUG("ALFANUMERICO "<<alpha);
                    } else LOG_DEBUG("ALFANUMERICO VUOTO");

                    k5=detail.find("<strong>Benchmark</strong>");
                    //<span class="t-text -right">MSCI BRAZIL INDEX</span>
                    k5=detail.find("<span class=\"t-text -right\">", k5);
                    k6=detail.find("</span>", k5);
                    string sector;
                    if (k5>0 && k6>0) {                        
                        sector=helpers::Trim(detail.substr(k5+28, k6-k5-28));
                        LOG_DEBUG("Benchmark "<< sector);
                    } else LOG_DEBUG("Benchmark VUOTO");

                    
                    k5=detail.find("<strong>Benchmark Style</strong>");
                    k5=detail.find("<span class=\"t-text -right\">", k5);
                    k6=detail.find("</span>", k5);
                    if (k5>0 && k6>0) {
                        sector=sector + ";"+helpers::Trim(detail.substr(k5+28, k6-k5-28));
                        LOG_DEBUG("Benchmark Style "<< sector);
                    } else sector = sector + ";";

                    k5=detail.find("<strong>Benchmark Area</strong>");
                    k5=detail.find("<span class=\"t-text -right\">", k5);
                    k6=detail.find("</span>", k5);
                    if (k5>0 && k6>0) {
                        sector=sector + ";"+trimstr(detail.substr(k5+28, k6-k5-28));
                        LOG_DEBUG("Benchmark Area "<< sector);
                    } else sector = sector + ";";
                    if (risultato.find(isin)==risultato.end() ) {
                        _map["ISIN"]=trimstr(isin);
                        _map["NAME"]=trimstr(nome);
                        _map["CODE"]=trimstr(alpha);
                        _map["SECTOR"]=trimstr(sector);
                        risultato[isin]=_map;
                        LOG_DEBUG("added "<<isin <<"\t"<<nome);
                    } else {ultimapagina=true;break;}
                    k1 = k4 ;
                }

                np++;
            }while (!ultimapagina);

        return risultato;
    }


void security::populate_db(string dbname) {
    security::loadDailyQuotesGoogle(dbname);
    security::loadDailyQuotesYahoo(dbname);
}

    security::security(string dbname,string symbol,string market,timeseries::freq_enum f):name(symbol),ts(init(dbname,symbol,market,f))
    {

/*
    Eigen::MatrixXd M;
    std::vector<time_t> dates;
    //string  freq;
        this->name=symbol;
      //  freq=freq;
        std::vector<std::string> tsnames;
        tsnames.push_back("open("+symbol+")");
        tsnames.push_back("high("+symbol+")");
        tsnames.push_back("low("+symbol+")");
        tsnames.push_back("close("+symbol+")");
        tsnames.push_back("volume("+symbol+")");
        tsnames.push_back("oi("+symbol+")");
        sqlite3 *db;
        sqlite3_stmt *statcnt;
        sqlite3_stmt *statement;
        int rc;
        unsigned len;
        string freq;
        if (f==timeseries::freq_enum::daily) freq="D";
        else if (f==timeseries::freq_enum::weekly) freq="W";
        else if (f==timeseries::freq_enum::monthly) freq="M";
        else THROW_EXCEPTION("frequence not found :"<<f);

        string querycnt = "SELECT count(*)  FROM SECURITIES WHERE NAME='"+symbol+"' and freq='"+freq+"';";
        string query = "SELECT *  FROM SECURITIES WHERE NAME='"+symbol+"' and freq='"+freq+"' order by date asc;";
        rc = sqlite3_open(dbname.c_str(), &db);
        if( rc )   THROW_EXCEPTION("cannot open database "<<dbname);

        if (sqlite3_prepare_v2(db,querycnt.c_str(),-1,&statcnt,NULL))
        {
            const char* c =sqlite3_errmsg(db);
            string  es(c);
            sqlite3_close(db);
            THROW_EXCEPTION("cannot create statement:"<<es);
        }

        rc = sqlite3_step (statcnt ) ;
        if (rc == SQLITE_ROW)
        {
            len=sqlite3_column_int(statcnt,0);
            sqlite3_finalize(statcnt);
        }
        else
        {
            sqlite3_finalize(statcnt);

            sqlite3_close(db);
            THROW_EXCEPTION("error fetching security "<<this->name);
        }

        if (len==0) THROW_EXCEPTION("no data for symbol "<<this->name);
        M.resize(len,6);//open,high,low,close,volume,oi

        if (sqlite3_prepare_v2(db,query.c_str(),-1,&statement,NULL))
        {
            const char* c =sqlite3_errmsg(db);
            string  es(c);
            sqlite3_close(db);
            THROW_EXCEPTION("cannot create statement:"<<es);
        }
        unsigned k=0;
        dates.clear();
        do
        {
            rc = sqlite3_step (statement ) ;
            if (rc == SQLITE_ROW)   // can read data
            {
                unsigned date = sqlite3_column_int(statement,2) ;
                double open = sqlite3_column_double(statement,3) ;
                double high = sqlite3_column_double(statement,4) ;
                double low = sqlite3_column_double(statement,5) ;
                double close = sqlite3_column_double(statement,6) ;
                double volume = sqlite3_column_double(statement,7) ;
                double oi = sqlite3_column_double(statement,8);
                dates.push_back(date);

                M(k,0)=open;
                M(k,1)=high;
                M(k,2)=low;
                M(k,3)=close;
                M(k,4)=volume;
                M(k,5)=oi;

                k++;
            }

        }
        while (rc == SQLITE_ROW);

        sqlite3_finalize(statement);
        sqlite3_close(db) ;
        this->ts=(const timeseries)timeseries(tsnames,dates,f,M);
        //LOG(DEBUG) << statement <<" "<<db;
        */
    }

    //const double security::open(time_t date) const {


    //}



const std::string security::getName() const{
return this->name;
}

    const double security::open(size_t i) const {
        //if (i>this->ts.getLength()) THROW_EXCEPTION("index out of bound :"<<i);
        return this->ts(i,0);

    }

    const double security::high(size_t i) const {
        //if (i>this->ts.getLength()) THROW_EXCEPTION("index out of bound :"<<i);
        return this->ts(i,1);

    }
    const double security::low(size_t i) const {
        //if (i>this->ts.getLength()) THROW_EXCEPTION("index out of bound :"<<i);
        return this->ts(i,2);

    }
    const double security::close(size_t i) const {
        //if (i>this->ts.getLength()) THROW_EXCEPTION("index out of bound :"<<i);
        return this->ts(i,3);
    }

    const double security::volume(size_t i) const {
        //if (i>this->ts.getLength()) THROW_EXCEPTION("index out of bound :"<<i);
        return this->ts(i,4);
    }
        const double security::oi(size_t i) const {
        //if (i>this->ts.getLength()) THROW_EXCEPTION("index out of bound :"<<i);
        return this->ts(i,5);
    }
    const double security::open(time_t d) const {
        return this->ts(ts.dateidx(d),0);
    }
    const double security::high(time_t d) const {
        return this->ts(ts.dateidx(d),1);
    }
    const double security::low(time_t d) const {
        return this->ts(ts.dateidx(d),2);
    }
    const double security::close(time_t d) const {
        return this->ts(ts.dateidx(d),3);
    }
    const double security::volume(time_t d) const {
        return this->ts(ts.dateidx(d),4);
    }
    const double security::oi(time_t d) const {
        return this->ts(ts.dateidx(d),5);
    }


    void security::initialize_db(std::string dbname)
    {
        std::ifstream infile(dbname);
        if (infile.good()) THROW_EXCEPTION("file "<< dbname << " exists, please choose another");
        security::createDB(dbname);security::populate_db_SECINFO(dbname);security::populate_db(dbname);

    }
