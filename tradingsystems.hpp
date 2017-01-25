#ifndef TRADINGSYSTEMS_HPP_INCLUDED
#define TRADINGSYSTEMS_HPP_INCLUDED
#include <iostream>
#include <cstdlib>
#include "logger.hpp"
#include "yahoofetch.hpp"
#include "security.h"
#include "netutils.hpp"
#include "backtesting.hpp"
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <curl/curl.h>
#include "googlefetch.hpp"
#include <sys/stat.h>
 #include <sys/types.h>

using namespace std;
namespace tradingsystems
{
int createDir(string dir ){
    struct stat st = {0};
    if (stat(dir.c_str(), &st) == -1) {
        int res=mkdir(dir.c_str(), 0700);
        return res;
    }
    return 0;
}
void checkCorrelation (string db,string tempdir="/tmp/tests/")
{
    ofstream of;
    if (createDir(tempdir)) THROW_EXCEPTION("cannot create dir "<<tempdir);
    of.open(tempdir+"test.txt");
    size_t minsample=500,window=60;
    vector<string> lv=security::getSecList(db,"","EUR","MLSE","STOCK");
    double mmaxv=0,mminv=0;
    string maxs="",mins="";
    of << "minsample "<<minsample<<" window "<<window;
    of.flush();
    for (auto &sym1 : lv)
    {
        try
        {
            timeseries ts1=security(db,sym1,"MLSE",timeseries::freq_enum::daily).ts.getserie(3);
            timeseries er1=ts1.er();
            if (er1.getDateGapInSeconds()/60/60/24>(7)) continue;//max 7 days gap
            if (er1.getLength()<minsample) continue;//min 500 samples
            if ( (time(0)-er1.getLastDate())> (60*60*24*14)) continue; //not too old
            mmaxv=0,mminv=0;
            maxs="",mins="";
            timeseries besttsmax,besttsmin;
            for (auto &sym2 : lv)
            {
                try
                {                    
                    timeseries ts2=security(db,sym2,"MLSE",timeseries::freq_enum::daily).ts.getserie(3);
                    if (ts2.getLength()<minsample) continue;                                       
                    timeseries er2d=ts2.er().mlagdelay(10);
                    timeseries ker2d=er2d.rowkron();
                    //timeseries kker2d=ker2d.rowkron();
                    //timeseries x=er2d.merge(ker2d).merge(kker2d);
                    timeseries x=er2d.merge(ker2d);                    
                    timeseries all=er1.merge(x);
                    if (all.getLength()<window) continue;
                    all=all.head(window);
                    if (all.getDateGapInSeconds()/60/60/24>(7)) continue;
                    Eigen::MatrixXd corr=all.corr();
                    double maxv=0,minv=0;
                    size_t maxidx=0,minidx=0;
                    for (long k=1; k<corr.cols(); k++)
                    {
                        if (k==1)
                        {
                            maxv=minv=corr(0,k);
                            maxidx=minidx=k;
                        }
                        else
                        {
                            if (maxv<corr(0,k))
                            {
                                maxv=corr(0,k);
                                maxidx=k;
                            }
                            if (minv>corr(0,k))
                            {
                                minv=corr(0,k);
                                minidx=k;
                            }
                        }
                    }
                    LOG_INFO("comparing "<<sym1<<"\t"<<sym2<<"\tmaxcorr="<<maxv<<"\tmincorr="<<minv);
                    if (mmaxv<maxv)
                    {
                        mmaxv=maxv;
                        maxs=all.getName(maxidx);
                        besttsmax=all.getserie(0).merge(all.getserie(maxidx));
                    }
                    if (mminv>minv)
                    {
                        mminv=minv;
                        mins=all.getName(minidx);
                        besttsmin=all.getserie(0).merge(all.getserie(minidx));
                    }
                    LOG_INFO("\nbest max="<<mmaxv<<" "<<maxs<<  "\n"<<"best min="<<mminv<<" "<<mins);
                }
                catch (std::exception &e)
                {
                    LOG_WARN(e.what());
                }
            }
            of << "\nSYMBOL "<<sym1;
            of <<"\nbest max="<<mmaxv<<" "<<maxs<<  "\n"<<"best min="<<mminv<<" "<<mins;
            of.flush();
            besttsmax.toCSV(tempdir+sym1+"_max.csv");
            besttsmin.toCSV(tempdir+sym1+"_min.csv");
        }
        catch (std::exception &e )
        {
            LOG_WARN(e.what());
        }
    }
    of.close();
}




void walkforwardsign(string symbol,size_t train_window=60,size_t test_win=5,size_t start=0)
{
    ofstream of;
    string db="db";
    of.open("/tmp/test_sign_"+symbol+".txt");
    size_t minsample=(train_window+test_win);
    timeseries ts1=security(db,symbol,"MLSE",timeseries::freq_enum::daily).ts.getserie(3);
    if (ts1.getLength()< minsample) THROW_EXCEPTION("too few data for symbol "<<symbol);
    timeseries er1=ts1.er();
    vector<string> lv=security::getSecList(db);
    of.flush();
    double toteq=1000;
    for (size_t i=start; i<(er1.getLength()-minsample); i=i+test_win )
    {
        timeseries er1w=er1.daterange(i,i+train_window-1);
        timeseries er1t=er1.daterange(i+train_window,i+train_window+test_win-1);
        string bestsacc,bestsdis;
        string bestsacc_l,bestsdis_l;
        size_t bestiacc=0,bestidis=0;
        double mmaxv=0,mminv=0;
        for ( auto &sym : lv)
        {
            try
            {
                timeseries ts2=security(db,sym,"MLSE",timeseries::freq_enum::daily).ts.getserie(3);
                if (ts2.getLength()<minsample) continue;
                LOG_INFO("check with symbol "<<sym);
                timeseries er2d=ts2.er().mlagdelay(5);
                //LOG_INFO(timeseries::date2string(er1w.getFirstDate() ) <<"\t"<<timeseries::date2string(er1w.getLastDate() ));
                //er2d=er2d.daterange(er1w.getFirstDate(),er1w.getLastDate());
                timeseries ker2d=er2d.rowkron();
                timeseries kker2d=ker2d.rowkron();
                timeseries x=er2d.merge(ker2d).merge(kker2d);
                timeseries all=er1w.merge(x);
                // if (all.getLength()<train_window) continue;

                Eigen::MatrixXd M=all.getMatrix();
                double  maxv=0;
                size_t maxidx=0;
                double  minv=0;
                size_t minidx=0;
                for (size_t j=1; j<all.getSeriesLen(); j++)
                {
                    size_t tm=0,ti=0;
                    Eigen::MatrixXd R=M.col(0).cwiseProduct(M.col(j));
                    for (long k=0; k<R.rows(); k++)
                    {
                        if (R(k)>0) tm++;
                        else if (R(k)<0) ti++;
                    }
                    double tmd=(double)tm/(double)train_window;
                    double tid=(double)ti/(double)train_window;
                    if (tmd  >maxv)
                    {
                        maxv=tmd;
                        maxidx=j;
                    }
                    if (tid  >minv)
                    {
                        minv=tid;
                        minidx=j;
                    }
                }
                if (mmaxv<maxv)
                {
                    mmaxv=maxv;
                    bestsacc=sym;
                    bestiacc=maxidx;
                    bestsacc_l=all.getName(maxidx);
                }
                if (mminv<minv)
                {
                    mminv=minv;
                    bestsdis=sym;
                    bestidis=minidx;
                    bestsdis_l=all.getName(minidx);
                }
                LOG_INFO("best maxaccord="<<mmaxv<<" "<<bestsacc<<"best maxdisaccord="<<mminv<<" "<<bestsdis);
                LOG_INFO(bestsacc_l<<"\t"<<bestsdis_l);

            }
            catch (std::exception &e)
            {
                LOG_WARN(e.what());
            }
        }

        //test best cross
        LOG_INFO("train from "<< timeseries::date2string(er1w.getFirstDate() )<<"\t"<<timeseries::date2string(er1w.getLastDate() ));
        LOG_INFO("test from "<<timeseries::date2string(er1t.getFirstDate() )<<"\t"<<timeseries::date2string(er1t.getLastDate()) );
        LOG_INFO("train samples "<<er1w.getLength());
        LOG_INFO("test samples "<<er1t.getLength());

        timeseries tsacc=security(db,bestsacc,"MLSE",timeseries::freq_enum::daily).ts.getserie(3).er().mlagdelay(5);
        timeseries tsdis=security(db,bestsdis,"MLSE",timeseries::freq_enum::daily).ts.getserie(3).er().mlagdelay(5);
        tsacc=tsacc.daterange(er1t.getFirstDate(),er1t.getLastDate());
        tsdis=tsdis.daterange(er1t.getFirstDate(),er1t.getLastDate());

        timeseries ktsacc=tsacc.rowkron();
        timeseries kktsacc=ktsacc.rowkron();
        timeseries x=tsacc.merge(ktsacc).merge(kktsacc);
        timeseries allacc=er1t.merge(x);

        timeseries ktsdis=tsdis.rowkron();
        timeseries kktsdis=ktsdis.rowkron();
        x=tsdis.merge(ktsdis).merge(kktsdis);
        timeseries alldis=er1t.merge(x);

        LOG_INFO("bestacc="<<allacc.getName(bestiacc)<<"\tbestdis="<<alldis.getName(bestidis));
        Eigen::MatrixXd Macc=allacc.getMatrix();
        Eigen::MatrixXd Mdis=alldis.getMatrix();
        Eigen::MatrixXd Racc=Macc.col(0).cwiseProduct(Macc.col(bestiacc));
        Eigen::MatrixXd Rdis=Mdis.col(0).cwiseProduct(Mdis.col(bestidis));
        double pacc=0,pdis=0;
        for (long k=0; k<Racc.rows(); k++)
        {
            if (Racc(k)>0) pacc+=Macc(k,0);
        }
        for (long k=0; k<Rdis.rows(); k++)
        {
            if (Rdis(k)<0) pdis+=Mdis(k,0);
        }
        //for (long k=0;k<Rdis.rows();k++) {if (Racc(k)>0 && Rdis(k)<0) ptwice+=Macc(k,0);}
        //pacc/=(double)test_win;pdis/=(double)test_win;ptwice/=(double)test_win;
        toteq=toteq *(1+ (pacc+pdis)/200) ;
        LOG_INFO("test acc="<<pacc<<"\ttest dis="<<pdis<<"\ttesttwice="<<toteq);

        std::cout<<"Press [Enter] to continue . . .";
        std::cin.get();
        std::cout.flush();

    }
    of.close();
}

void checkSign (string db,string tempdir="/tmp/")
{
    ofstream of;
    if (createDir(tempdir)) THROW_EXCEPTION("cannot create dir "<<tempdir);
    of.open(tempdir+"test.txt");


    size_t minsample=500,window=60;
    vector<string> lv=security::getSecList(db,"","EUR","MLSE","STOCK");
    vector<string> lv2=security::getSecList(db,"","EUR","MLSE");
    double mmaxv=0,mminv=0;
    string maxs="",mins="";
    of << "minsample "<<minsample<<" window "<<window;
    of.flush();
    for (auto &sym1 : lv)
    {
        try
        {
            timeseries ts1=security(db,sym1,"MLSE",timeseries::freq_enum::daily).ts.getserie(3);
            timeseries er1=ts1.er();
            if (ts1.getLength()<minsample) continue;
            mmaxv=0;
            mminv=0;
            maxs="";
            mins="";
            timeseries besttsmax,besttsmin;
            for (auto &sym2 : lv2)
            {
                try
                {
                    timeseries ts2=security(db,sym2,"MLSE",timeseries::freq_enum::daily).ts.getserie(3);
                    if (ts2.getLength()<minsample) continue;
                    timeseries er2d=ts2.er().mlagdelay(7);
                    timeseries ker2d=er2d.rowkron();
                    timeseries kker2d=ker2d.rowkron();
                    timeseries x=er2d.merge(ker2d).merge(kker2d);
                    timeseries all=er1.merge(x);
                    if (all.getLength()<window) continue;
                    all=all.head(window);
                    if (all.getDateGapInSeconds()/60/60/24>7) continue;
                    Eigen::MatrixXd M=all.getMatrix();
                    double  maxv=0;
                    size_t maxidx=0;
                    double  minv=0;
                    size_t minidx=0;
                    for (size_t j=1; j<all.getSeriesLen(); j++)
                    {
                        size_t tm=0,ti=0;
                        Eigen::MatrixXd R=M.col(0).cwiseProduct(M.col(j));
                        for (long k=0; k<R.rows(); k++)
                        {
                            if (R(k)>0) tm++;
                            else if (R(k)<0) ti++;
                        }
                        double tmd=(double)tm/(double)window;
                        double tid=(double)ti/(double)window;
                        if (tmd  >maxv)
                        {
                            maxv=tmd;
                            maxidx=j;
                        }
                        if (tid  >minv)
                        {
                            minv=tid;
                            minidx=j;
                        }
                    }

                    LOG_INFO("comparing "<<sym1<<"\t"<<sym2<<"\tmaxaccord="<<maxv<<"\tmaxdisaccord="<<minv);
                    if (mmaxv<maxv)
                    {
                        mmaxv=maxv;
                        maxs=all.getName(maxidx);
                        besttsmax=all.getserie(0).merge(all.getserie(maxidx)).merge(ts2);
                    }
                    if (mminv<minv)
                    {
                        mminv=minv;
                        mins=all.getName(minidx);
                        besttsmin=all.getserie(0).merge(all.getserie(minidx)).merge(ts2);
                    }

                    LOG_INFO("\nbest maxaccord="<<mmaxv<<" "<<maxs<<"\nbest maxdisaccord="<<mminv<<" "<<mins);
                }
                catch (std::exception &e)
                {
                    LOG_WARN(e.what());
                }
            }
            of << "\nSYMBOL="<<sym1;
            of <<" best maxaccord="<<mmaxv<<" "<<maxs<<" best maxdisaccord="<<mminv<<" "<<mins;
            of.flush();
            besttsmax.merge(ts1).toCSV(tempdir+sym1+"_maxacc.csv");
            besttsmin.merge(ts1).toCSV(tempdir+sym1+"_maxdisacc.csv");
        }
        catch (...) {}

    }
    of.close();
}

void tradingsystem(string db,string symbol)
{
    security sec=security(db,symbol,"MLSE",timeseries::freq_enum::daily);
    timeseries close =sec.ts.getserie(3);
    timeseries er=close.er();
    timeseries sharpe=er.sharpe(20).sma(200);
    //sharpe.plot();
    close.plot();
    vector<trading_order> sign;
    bool flat=true;
    double treshold=.01;
    for (size_t i=0; i<(sharpe.getLength()-2 ); i++)
    {

        if (sharpe(i,0)>treshold && flat)
        {
            trading_order o1;
            o1.tsign=trading_order::trading_signal::buyatopen;
            o1.date=sharpe.getDateTime(i+1);
            sign.push_back(o1);
            flat=false;
        }
        if (sharpe(i,0)<-treshold && !flat)
        {
            trading_order o2;
            o2.tsign=trading_order::trading_signal::sellatopen;
            o2.date=sharpe.getDateTime(i+1);
            sign.push_back(o2);
            flat=true;
        }

    }
    //statistics s=backtesting(sec,sign,sec.ts.getFirstDate(),sec.ts.getLastDate(),0.02,0.02,1000,.26);
    statistics s=statistics::backtesting(sec,sign,sec.ts.getFirstDate(),sec.ts.getLastDate(),0.00,0.0,1000,.0);
    LOG_INFO(s.toString());
    s.equity.plot();
}

const statistics tradingsystemLS(string db,string symbol,double treshold=0.05)
{
    security sec=security(db,symbol,"MLSE",timeseries::freq_enum::daily);
    timeseries close =sec.ts.getserie(3);
    timeseries er=close.er();
    timeseries sharpe=er.sharpe(20).sma(200);
    //sharpe.plot();

    vector<trading_order> sign;
    bool flat=true,longshort=true;

    for (size_t i=0; i<(sharpe.getLength()-2 ); i++)
    {

        if (sharpe(i,0)>treshold )
        {
            if (flat)
            {
                trading_order o1;
                o1.tsign=trading_order::trading_signal::buyatopen;
                o1.date=sharpe.getDateTime(i+1);
                sign.push_back(o1);
            }
            else
            {
                if (!longshort)
                {
                    trading_order oc;
                    oc.tsign=trading_order::trading_signal::coveratopen;
                    oc.date=sharpe.getDateTime(i+1);
                    sign.push_back(oc);
                    trading_order o1;
                    o1.tsign=trading_order::trading_signal::buyatopen;
                    o1.date=sharpe.getDateTime(i+1);
                    sign.push_back(o1);
                }
            }
            flat=false;
            longshort=true;

        }
        if (sharpe(i,0)<-treshold )
        {
            if (flat)
            {
                trading_order o1;
                o1.tsign=trading_order::trading_signal::shortatopen;
                o1.date=sharpe.getDateTime(i+1);
                sign.push_back(o1);

            }
            else
            {
                if (longshort)
                {
                    trading_order oc;
                    oc.tsign=trading_order::trading_signal::sellatopen;
                    oc.date=sharpe.getDateTime(i+1);
                    sign.push_back(oc);
                    trading_order o1;
                    o1.tsign=trading_order::trading_signal::shortatopen;
                    o1.date=sharpe.getDateTime(i+1);
                    sign.push_back(o1);
                }

            }
            longshort=false;
            flat=false;

        }

    }
    //statistics s=backtesting(sec,sign,sec.ts.getFirstDate(),sec.ts.getLastDate(),0.02,0.02,1000,.26);
    //const statistics backtesting (const security& sec,const vector<trading_order>& sign,time_t from, time_t to,double longfeepc,double shortfeepc,double initeq,double tax);
    statistics s=statistics::backtesting(sec,sign,sec.ts.getFirstDate(),sec.ts.getLastDate(),0.00,0.0,1000,.0);
    LOG_INFO(s.toString());
    return s;
    //s.equity.plot();
}





}
#endif // TRADINGSYSTEMS_HPP_INCLUDED
