#include "timeseries.hpp"
#include "security.h"
#include <vector>
#include "backtesting.hpp"
//return equity
const statistics statistics::backtesting (const security& sec,const vector<trading_order>& sign,time_t from, time_t to,double longfeepc,double shortfeepc,double initeq,double tax)
{
    if (from <sec.ts.getFirstDate() || to > sec.ts.getLastDate()) THROW_EXCEPTION("bed begin end times: "<<from << "\t"<<to);
    size_t tstart=sec.ts.dateidx(from),tstop=sec.ts.dateidx(to);
    statistics stats;

    size_t ntrades,nwin=0,nlose=0,nlong=0,nshort=0;
    vector<time_t> equity_dates;
    vector<double> equity_values;
    bool flat=true,longshort=true;
    double trigger=0,totalfee=0;
    vector<trading_order>::const_iterator itord=sign.begin();
    vector<trading_order>::const_iterator currentord;
    size_t i=tstart;
    for (itord=sign.begin(); itord!=sign.end(); itord++)
    {
        trading_order tmpord=*itord;
        LOG_DEBUG("Order "<< trading_order::ts2string(tmpord.tsign)  <<" at "<<timeseries::date2string(tmpord.date)<<" for "<<sec.getName());
        while (itord->date!=sec.ts.getDateTime(i))
        {
            if (flat)
            {
                if (i==tstart) equity_values.push_back(initeq);
                else
                    equity_values.push_back(equity_values.at(equity_values.size()-1));
                equity_dates.push_back(sec.ts.getDateTime(i));
            }
            else
            {
                double delta=(1+(longshort?1:-1)*(sec.close(i)-sec.close(i-1))/sec.close(i-1) );

                equity_values.push_back(delta*equity_values.back());

                equity_dates.push_back(sec.ts.getDateTime(i));
            }
            i++;
        }

        if (itord==sign.begin())   //first order
        {

            switch (itord->tsign)
            {
            case (trading_order::trading_signal::buyatclose):
            {
                longshort=true;
                flat=false;
                trigger=equity_values.size()>0?equity_values.back():initeq;
                break;

            }
            case (trading_order::trading_signal::buyatopen):
            {
                longshort=true;
                flat=false;
                trigger=equity_values.size()>0?equity_values.back():initeq;
                double delta=(1+(sec.close(i)-sec.open(i))/sec.open(i) );// *equity_values.at(equity_values.size()-1);
                if (i==tstart)
                {
                    equity_values.push_back(delta*initeq);
                    equity_dates.push_back(sec.ts.getDateTime(i));
                }
                else
                {
                    equity_values.push_back(delta*equity_values.at(equity_values.size()-1));
                    equity_dates.push_back(sec.ts.getDateTime(i));
                }

                break;
            }
            case (trading_order::trading_signal::shortatclose) :
            {
                longshort=false;
                flat=false;
                trigger=equity_values.size()>0?equity_values.back():initeq;
                break;
            }
            case (trading_order::trading_signal::shortatopen):
            {
                longshort=false;
                flat=false;
                trigger=equity_values.size()>0?equity_values.back():initeq;
                double delta=(1-(sec.close(i)-sec.open(i))/sec.open(i) );// *equity_values.at(equity_values.size()-1);
                if (i==tstart)
                {
                    equity_values.push_back(delta*initeq);
                }
                else
                {
                    equity_values.push_back(delta*equity_values.back());
                    equity_dates.push_back(sec.ts.getDateTime(i));
                }

                break;
            }
            default:
                THROW_EXCEPTION("beginning signal not valid "<<itord->date<<":"<<itord->tsign);
                break;
            }

        }
        else
        {
            if (itord->date<currentord->date) THROW_EXCEPTION("order before last order "<<itord->date<<":"<<itord->tsign);
            switch (itord->tsign)
            {
            case (trading_order::trading_signal::buyatclose):
            {
                if (!flat) THROW_EXCEPTION("bad signal at "<<timeseries::date2string(itord->date)<<":"<<trading_order::ts2string(itord->tsign));
                longshort=true;
                flat=false;
                trigger=equity_values.size()>0?equity_values.back():initeq;
                break;
            }
            case (trading_order::trading_signal::buyatopen):
            {
                if (!flat) THROW_EXCEPTION("bad signal at "<<timeseries::date2string(itord->date)<<":"<<trading_order::ts2string(itord->tsign));
                longshort=true;
                flat=false;
                trigger=equity_values.size()>0?equity_values.back():initeq;
                double delta=(1+(sec.close(i)-sec.open(i))/sec.open(i) );
                equity_values.push_back(delta*equity_values.at(equity_values.size()-1));
                equity_dates.push_back(sec.ts.getDateTime(i));

                break;

            }
            case (trading_order::trading_signal::shortatclose) :
            {
                if (!flat) THROW_EXCEPTION("bad signal at "<<timeseries::date2string(itord->date)<<":"<<trading_order::ts2string(itord->tsign));
                longshort=false;
                flat=false;
                trigger=equity_values.size()>0?equity_values.back():initeq;
                break;
            }
            case (trading_order::trading_signal::shortatopen):
            {
                if (!flat) THROW_EXCEPTION("bad signal at "<<timeseries::date2string(itord->date)<<":"<<trading_order::ts2string(itord->tsign));
                longshort=false;
                flat=false;
                trigger=equity_values.size()>0?equity_values.back():initeq;
                double delta=(1-(sec.close(i)-sec.open(i))/sec.open(i) );
                equity_values.push_back(delta*equity_values.back());
                equity_dates.push_back(sec.ts.getDateTime(i));

                break;
            }

            case (trading_order::trading_signal::sellatclose):
            {
                if (flat || !longshort) THROW_EXCEPTION("bad signal at "<<timeseries::date2string(itord->date)<<":"<<trading_order::ts2string(itord->tsign));
                flat=true;

                if (currentord->tsign==trading_order::trading_signal::buyatopen && currentord->date==itord->date)
                {
                    double delta=(1+(sec.close(i)-sec.open(i))/sec.open(i)  );
                    double fee=(delta*equity_values.back()-trigger)>0?(delta*equity_values.back()-trigger)*tax:0 +(delta*equity_values.back()+trigger)*longfeepc;
                    totalfee+=fee;
                    equity_values.push_back(delta*equity_values.back()-fee);
                    equity_dates.push_back(sec.ts.getDateTime(i));
                    if (delta>0) nwin++;
                    else nlose++;
                }
                else
                {
                    double delta=(1+(sec.close(i)-sec.close(i-1))/sec.close(i-1) );
                    double fee=(delta*equity_values.back()-trigger)>0?(delta*equity_values.back()-trigger)*tax:0 +(delta*equity_values.back()+trigger)*longfeepc;
                    //double fee=(sec.close(i)-trigger)>0?(delta*equity_values.back()-trigger)*tax:0 +(sec.close(i)+trigger)*longfeepc;
                    totalfee+=fee;
                    equity_values.push_back(delta*equity_values.back()-fee);
                    equity_dates.push_back(sec.ts.getDateTime(i));
                    if (currentord->tsign==trading_order::trading_signal::buyatopen)
                    {
                        if ( (sec.close(i)-sec.open(currentord->date))>0) nwin++;
                        else nlose++;
                    }
                    else
                    {
                        if ( (sec.close(i)-sec.close(currentord->date))>0) nwin++;
                        else nlose++;
                    }

                }
                nlong++;
                break;
            }

            case (trading_order::trading_signal::sellatopen):
            {
                if (flat || !longshort) THROW_EXCEPTION("bad signal at "<<timeseries::date2string(itord->date)<<":"<<trading_order::ts2string(itord->tsign));
                flat=true;
                if (currentord->tsign==trading_order::trading_signal::buyatopen && currentord->date==itord->date)
                {
                    THROW_EXCEPTION("bad concurrent signal at "<<itord->date<<":"<<itord->tsign);
                }
                else
                {
                    double delta=(1+(sec.open(i)-sec.close(i-1))/sec.close(i-1) );
                    double fee=(delta*equity_values.back()-trigger)>0?(delta*equity_values.back()-trigger)*tax:0 +(delta*equity_values.back()+trigger)*longfeepc;
                    //double fee=(sec.open(i)-trigger)>0?(delta*equity_values.back()-trigger)*tax:0 +(sec.open(i)+trigger)*longfeepc;
                    totalfee+=fee;
                    equity_values.push_back(delta*equity_values.back()-fee);
                    equity_dates.push_back(sec.ts.getDateTime(i));
                    if (currentord->tsign==trading_order::trading_signal::buyatopen)
                    {
                        if ( (sec.open(i)-sec.open(currentord->date))>0) nwin++;
                        else nlose++;
                    }
                    else
                    {
                        if ( (sec.open(i)-sec.close(currentord->date))>0) nwin++;
                        else nlose++;
                    }

                }
                nlong++;
                break;
            }
            case (trading_order::trading_signal::coveratclose):
            {
                if (flat || longshort) THROW_EXCEPTION("bad signal at "<<timeseries::date2string(itord->date)<<":"<<trading_order::ts2string(itord->tsign));
                flat=true;
                if (currentord->tsign==trading_order::trading_signal::shortatopen && currentord->date==itord->date)
                {
                    double delta=(1-(sec.close(i)-sec.open(i))/sec.open(i)  );
                    double fee=(delta*equity_values.back()-trigger)<0?-(delta*equity_values.back()-trigger)*tax:0 +(delta*equity_values.back()+trigger)*shortfeepc;
                    //double fee=(sec.close(i)-trigger)<0?fabs(delta*equity_values.back()-trigger)*tax:0 +(sec.close(i)+trigger)*shortfeepc;
                    totalfee+=fee;
                    equity_values.push_back(delta*equity_values.back()-fee);
                    equity_dates.push_back(sec.ts.getDateTime(i));
                    if (delta<0) nwin++;
                    else nlose++;
                }
                else
                {
                    double delta=(1-(sec.close(i)-sec.close(i-1))/sec.close(i-1)  );
                    double fee=(delta*equity_values.back()-trigger)<0?-(delta*equity_values.back()-trigger)*tax:0 +(delta*equity_values.back()+trigger)*shortfeepc;
                    //double fee=(sec.close(i)-trigger)<0?fabs(delta*equity_values.back()-trigger)*tax:0 +(sec.close(i)+trigger)*shortfeepc;
                    totalfee+=fee;
                    equity_values.push_back(delta*equity_values.back()-fee);
                    equity_dates.push_back(sec.ts.getDateTime(i));
                    if (currentord->tsign==trading_order::trading_signal::shortatopen)
                    {
                        if ( (sec.close(i)-sec.open(currentord->date))<0) nwin++;
                        else nlose++;
                    }
                    else
                    {
                        if ( (sec.close(i)-sec.close(currentord->date))<0) nwin++;
                        else nlose++;
                    }

                }
                nshort++;
                break;
            }

            case (trading_order::trading_signal::coveratopen):
            {
                if (flat || longshort) THROW_EXCEPTION("bad signal at "<<timeseries::date2string(itord->date)<<":"<<trading_order::ts2string(itord->tsign));
                flat=true;
                if (currentord->tsign==trading_order::trading_signal::shortatopen && currentord->date==itord->date)
                {
                    THROW_EXCEPTION("bad concurrent signal at "<<itord->date<<":"<<itord->tsign);
                }
                else
                {
                    double delta=(1-(sec.open(i)-sec.close(i-1))/sec.close(i-1) );
                    double fee=(delta*equity_values.back()-trigger)<0?-(delta*equity_values.back()-trigger)*tax:0 +(delta*equity_values.back()+trigger)*shortfeepc;
                    //double fee=(sec.close(i)-trigger)<0?fabs(delta*equity_values.back()-trigger)*tax:0 +(sec.open(i)+trigger)*shortfeepc;
                    totalfee+=fee;
                    equity_values.push_back(delta*equity_values.back()-fee);
                    equity_dates.push_back(sec.ts.getDateTime(i));
                    if (currentord->tsign==trading_order::trading_signal::shortatopen)
                    {
                        if ( (sec.open(i)-sec.open(currentord->date))<0) nwin++;
                        else nlose++;
                    }
                    else
                    {
                        if ( (sec.open(i)-sec.close(currentord->date))<0) nwin++;
                        else nlose++;
                    }
                }
                nshort++;
                break;
            }

            default:
                THROW_EXCEPTION("signal not valid "<<itord->date<<":"<<itord->tsign);
                break;
            }

        }
        currentord=itord;
    }


    while (i<=tstop)
    {
        if (flat)
        {
            if (i==tstart)
            {
                equity_values.push_back(initeq);
                equity_dates.push_back(sec.ts.getDateTime(i));
            }
            else
            {
                equity_values.push_back(equity_values.back());
                equity_dates.push_back(sec.ts.getDateTime(i));
            }
        }
        else
        {
            if (i==tstart)
            {
                if (sign.back().tsign==trading_order::trading_signal::buyatopen || sign.back().tsign==trading_order::trading_signal::shortatopen)
                {
                    double delta=(1+(longshort?1:-1)*(sec.close(i)-sec.open(i))/sec.open(i) ) ;
                    equity_dates.push_back(sec.ts.getDateTime(i));
                    equity_values.push_back(delta*initeq);
                }
            }
            else
            {
                if ( (sign.back().tsign==trading_order::trading_signal::buyatopen || sign.back().tsign==trading_order::trading_signal::shortatopen) && sign.back().date==sec.ts.getDateTime(i) )
                {
                    double delta=(1+(longshort?1:-1)*(sec.close(i)-sec.open(i))/sec.open(i) );
                    equity_dates.push_back(sec.ts.getDateTime(i));
                    equity_values.push_back(delta*equity_values.back());
                }
                else
                {
                    double delta=(1+(longshort?1:-1)*(sec.close(i)-sec.close(i-1))/sec.close(i-1) );
                    equity_dates.push_back(sec.ts.getDateTime(i));
                    equity_values.push_back(delta*equity_values.back());
                }
            }
        }
        //LOG_INFO("eq = "<<equity_values.back()<<"\t"<<equity_dates.back());
        i++;

    }


//correggere fee
    if (!flat)
    {
        double fee=0;
        if (longshort)
        {
            nlong++;

            if (sign.back().tsign==trading_order::trading_signal::buyatopen)
            {
                if ( (sec.close(tstop)-sec.open(sign.back().date))>0 )
                {
                    nwin++;
                    fee=fabs(equity_values.back()-trigger)*tax ;//-(sec.open(i)+trigger)*shortfeepc;
                }
                else nlose++;
                fee+=(equity_values.back()+trigger)*longfeepc;

            }
            else
            {
                if ( (sec.close(tstop)-sec.close(sign.back().date))>0 )
                {
                    nwin++;
                    fee=fabs(equity_values.back()-trigger)*tax ;
                }
                else nlose++;
                fee+=(equity_values.back()+trigger)*longfeepc;
            }
        }
        else
        {
            nshort++;
            if (sign.back().tsign==trading_order::trading_signal::shortatopen)
            {
                if ( (sec.close(tstop)-sec.open(sign.back().date))<0 )
                {
                    nwin++;
                    fee=fabs(equity_values.back()-trigger)*tax;
                }
                else nlose++;
                fee+=(equity_values.back()+trigger)*shortfeepc;
            }
            else if ( (sec.close(tstop)-sec.close(sign.back().date))<0 )
            {
                nwin++;
                fee=fabs(equity_values.back()-trigger)*tax;
            }
            else nlose++;
            fee+=(equity_values.back()+trigger)*shortfeepc;
        }
        double teq=equity_values.back();
        equity_values.pop_back();
        equity_values.push_back(teq-fee);
        totalfee+=fee;
    }
    stats.netprofit=equity_values.back()-initeq;
    stats.grossprofit=stats.netprofit+totalfee;
    ntrades=nshort+nlong;
    //equity_values


    //compress equity (same date)
    vector<time_t> newdates;
    vector<double> newval;

    for (size_t k=0; k<equity_dates.size(); k++)
    {
        if (k==0)
        {
            newdates.push_back(equity_dates.at(k));
            newval.push_back(equity_values.at(k));
        }
        else
        {
            if (equity_dates.at(k)==equity_dates.at(k-1))
            {
                //double t=newval.back();
                newval.pop_back();
                newval.push_back(equity_values.at(k));
            }
            else
            {
                newdates.push_back(equity_dates.at(k));
                newval.push_back(equity_values.at(k));
            }

        }
        //  LOG_INFO("newval "<<newval.back()<<"\t"<<newdates.back());

    }
    //


    Eigen::MatrixXd M(newval.size(),1);
    vector<string> eqname;
    eqname.push_back(sec.getName()+ " equity");
    for (size_t k=0; k<newval.size(); k++)  M(k,0)=newval.at(k);

    stats.equity=timeseries(eqname,newdates,sec.ts.getFreq(),M);
    //maxdd
    double mdd=0,mddp=0,maxmdd=initeq;
    for (size_t i=1; i<newval.size(); i++)
    {
        if (newval[i]>=newval[i-1])
        {
            if (newval[i]>maxmdd) maxmdd = newval[i];
            continue;
        }
        double t1=newval[i]-maxmdd;
        double t1p=t1/maxmdd;
        if (t1<mdd) mdd=t1;
        if (t1p<mddp) mddp=t1p;

    }
    stats.maxdrawdown_percent=mddp;
    stats.maxdrawdown_points=mdd;
    //regression slopw
    /*/maxDD calculus

    //regression slope
    double meanx=0,meany=0,varx=0,vary=0,covxy=0;
    for (int i=0;i<equityval.length;i++){meany+=equityval[i][0];meanx+=i;}
    meany/=(double)dates.length;meanx/=(double)equityval.length;
    for (int i=0;i<equityval.length;i++)  {varx+=Math.pow(i-meanx,2);vary+=Math.pow(equityval[i][0]-meany,2);}
    vary/=(double)(equityval.length-1);varx/=(double)(equityval.length-1);
    for (int i=0;i<equityval.length;i++) covxy+=(i-meanx)*(equityval[i][0]-meany);covxy/=(double)(equityval.length-1);
    double reg_b=covxy/varx;
    double reg_a=meany-reg_b*meanx;
    //double [] equityreg=new double[equityval.length];
    for (int i=0;i<equityval.length;i++) equityval[i][1]=reg_a+reg_b*i;
    java.util.ArrayList<String> eqname=new java.util.ArrayList<String>();
    eqname.add("equity");eqname.add("linreg");
    //stats.equity.add("linreg",equityreg);
    stats.linregslope=reg_b;
    */


    //cout <<stats.equity;
    stats.ntrades=ntrades;
    stats.nlong=nlong;
    stats.nlose=nlose;
    stats.nshort=nshort;
    stats.nwin=nwin;
    return stats;
}
