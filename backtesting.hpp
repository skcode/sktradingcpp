#ifndef BACKTESTING_HPP_INCLUDED
#define BACKTESTING_HPP_INCLUDED

#include "timeseries.hpp"
#include "security.h"
#include <vector>
using namespace std;

class trading_order
{
public:


    enum trading_signal {buyatclose=0,sellatclose=1,buyatopen=2,sellatopen=3,shortatclose=4,coveratclose=5,shortatopen=6,coveratopen=7};
    static const string ts2string(trading_signal ts)
    {
        static  const char *  trading_signal_string[]= {"buyatclose","sellatclose","buyatopen","sellatopen","shortatclose","coveratclose","shortatopen","coveratopen"};
        return trading_signal_string[ts];
    }

    time_t date;
    trading_signal tsign;
};

class statistics
{

private :
    string time2string(const time_t& t)
    {


        std::tm * ptm = std::localtime(&t);
        char buffer[32];
// Format: Mo, 15.06.2009 20:20:00
        std::strftime(buffer, 32, "%a, %d.%m.%Y %H:%M:%S", ptm);
        return string(buffer);
    }
public:
    timeseries equity;
    size_t ntrades,nwin,nlose,nlong,nshort;
    double netprofit,grossprofit;
    double maxdrawdown_percent,maxdrawdown_points;

    std::string toString()
    {
        std::stringstream ss;
        ss<<"\nntrades="<<ntrades<<"\tnwin="<<nwin<<"\tnlose"<<nlose<<"\tnlong="<<nlong<<"\tnshort="<<nshort;
        ss<<"\nnetprofit="<<netprofit<<"\tgrossprofit="<<grossprofit;
        ss<<"\nmaxdd%="<<maxdrawdown_percent<<"\tmaxddpoints="<<maxdrawdown_points;
        ss<<"\ninitequity="<<equity(0,0)<<"\tlastequity="<<equity(equity.getLength()-1,0);
        ss<<"\nfirstdate="<<time2string( equity.getFirstDate() )<<"\tlastdate="<<time2string( equity.getLastDate() );
        return ss.str();
    }
    /**crea le statistiche di backtesting per la security in input e la serie di ordini
    * @param sec security e.g. ENI.MI
    * @param sign trading_order vector
    * @param from starting time of trading
    * @param to ending time of trading
    * @param longfeepc percent fee of long trading e.g. 0.005
    * @param shortfeepc percent fee of short trading e.g. 0.005
    * @param initeq starting equity e.g. 10000
    * @param tax capital gain tax e.g. 0.26
    * @return statistics object
    */
     static const statistics backtesting (const security& sec,const vector<trading_order>& sign,time_t from, time_t to,double longfeepc,double shortfeepc,double initeq,double tax);
};

//return equity

//const statistics backtesting (const security& sec,const vector<trading_order>& sign,time_t from, time_t to,double longfeepc,double shortfeepc,double initeq,double tax);


#endif // BACKTESTING_HPP_INCLUDED
