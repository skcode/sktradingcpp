#include <curl/curl.h>
#include "logger.hpp"
#include <string>
#include <vector>
#include "netutils.hpp"
#include "googlefetch.hpp"
#include <algorithm>
using namespace std;

namespace googlefetch
{




std::string TrimLeft(const std::string& s)
{
    const std::string WHITESPACE = " \n\r\t";
    size_t startpos = s.find_first_not_of(WHITESPACE);
    return (startpos == std::string::npos) ? "" : s.substr(startpos);
}

std::string TrimRight(const std::string& s)
{
    const std::string WHITESPACE = " \n\r\t";
    size_t endpos = s.find_last_not_of(WHITESPACE);
    return (endpos == std::string::npos) ? "" : s.substr(0, endpos+1);
}


std::string Trim(const std::string& s)
{
    return TrimRight(TrimLeft(s));
}


vector<vector<string > > googlefetch2(string symbol,string _market)
{
    static const char * months[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    size_t start=0;

    //http://www.google.com/finance/historical?q=BIT%3AENEL&startdate=Jan%201%2C%202000&enddate=Dic%208%2C%202014&num=30
    //http://www.google.com/finance/historical?q=BIT%3AENEL&startdate=Jan%201%2C%202000&enddate=Dic%208%2C%202014&num=200&start=200

    vector<vector<string > > values;
    vector<string >  record;
    string market;
    if (_market=="MLSE") market="BIT";
    else  if (_market=="NYSE")  market="NYSE";
    else  if (_market=="NASDAQ")  market="NASDAQ";
    else THROW_EXCEPTION("unknown market "<<market);
    string baseurl="http://www.google.com/finance/historical?q="+market+"%3A"+symbol+"&startdate=Jan%201%2C%202000&enddate=Dic%2031%2C%202099&num=200&start=";// +std::to_string(start);
    while (true)
    {
        //cout <<(baseurl+std::to_string(start));
        string res=netutils::httpfetch(baseurl+std::to_string(start));

        size_t t1=res.find("<table class=\"gf-table historical_price\">");
        if (t1==string::npos) break;
        size_t tend=res.find("</table>",t1)+5;
        size_t k=0;
        while (t1<tend)
        {
            string tok="";
            size_t t2,t3;
            t2=res.find(">",t1);
            t3=res.find("<",t2);
            tok=res.substr(t2+1,t3-t2-2);
            t1=t3+1;
            tok=Trim(tok);
            if (tok.size()<1) continue;
            if (tok=="Date" || tok=="Open" || tok=="High" || tok=="Low" || tok=="Close" || tok=="Volume") continue;
            //cout <<"\n"<<tok;
            //if (t1<tend)
            //{


            if (k==0)  //date "Sep 29, 2014" "Oct 1, 2014"
            {
                long midx=-1;
                string m,y,d;
                for (size_t i=0; i<12; i++) if (tok.substr(0,3)==months[i] ) midx=i;
                //if (midx<0) THROW_EXCEPTION("bad date format "<< tok);
                if (midx>=0)
                {
                    midx++;//base 1
                    if (midx<10) m="0"+std::to_string(midx);
                    else m=std::to_string(midx);
                    if (tok.size()==12) d=tok.substr(4,2);
                    else d="0"+tok.substr(4,1);
                    if (tok.size()==12) y=tok.substr(8,4);
                    else y=tok.substr(7,4);
                    record.push_back(symbol);
                    record.push_back(_market);
                    record.push_back("D");
                    record.push_back(y+m+d);
                    //LOG_DEBUG("DATA"<<record.back());
                    k++;

                }
                else
                {
                    LOG_WARN("bad date "<<tok);
                    record.clear();
                    continue;
                }
            }
            else
            {

                bool isn=true;
                try
                {
                    std::stod(tok);
                }
                catch (...)
                {
                    isn=false;
                    LOG_WARN("numero errato "<<tok);
                }
                if (isn)
                {
                    record.push_back(tok);
                    //LOG_DEBUG("NUMERO"<<record.back());
                }
                else
                {
                    record.clear();
                    k=0;
                    continue;
                }
                if (k==5)
                {
                    record.push_back("0.0");//open interest =0
                    values.push_back(record);
                    record.clear();
                    k=0;
                }
                else k++;
            }

            //if (record.size()>0) cout <<"\n"<<k<<"*"<<record.back()<<"*";
            //t1=t3+1;
            //}
        }
        start+=200;
    }
    return values;

}

vector<vector<string > > googlefetch(string symbol,string _market)
{
    static const char * months[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    size_t start=0;

    //https://www.google.com/finance/historical?q=BIT%3ABMPS&startdate=Jan+1%2C+2000&num=200&start=0
    vector<vector<string > > values;
    vector<string >  record;
    string market;
    if (_market=="MLSE") market="BIT";
    else  if (_market=="NYSE")  market="NYSE";
    else  if (_market=="NASDAQ")  market="NASDAQ";
    else THROW_EXCEPTION("unknown market "<<market);
    string baseurl="http://www.google.com/finance/historical?q="+market+"%3A"+symbol+"&startdate=Jan+1%2C+2000&num=200&start=";// +std::to_string(start);
    while (true)
    {
        //cout <<(baseurl+std::to_string(start));
        string res=netutils::httpfetch(baseurl+std::to_string(start));

        size_t t1=res.find("<table class=\"gf-table historical_price\">");
        if (t1==string::npos) break;

        string tkdatestart="<td class=\"lm\">",tkend="<";
        string tkvalstart="<td class=\"rgt\">",tkvolstart="<td class=\"rgt rm\">";

        bool cicle=true;
        while (cicle)
        {

            record.clear();
            size_t t2,t3;
            t2=res.find(tkdatestart,t1);
            t3=res.find(tkend,t2+1);
            string tokdate=Trim(res.substr(t2+tkdatestart.size(),t3-t2-tkdatestart.size()-1));

            long midx=-1;
            string m,y,d;
            for (size_t i=0; i<12; i++) if (tokdate.substr(0,3)==months[i] ) midx=i;
            //if (midx<0) THROW_EXCEPTION("bad date format "<< tok);
            if (midx>=0)
            {
                midx++;//base 1
                if (midx<10) m="0"+std::to_string(midx);
                else m=std::to_string(midx);
                if (tokdate.size()==12) d=tokdate.substr(4,2);
                else d="0"+tokdate.substr(4,1);
                if (tokdate.size()==12) y=tokdate.substr(8,4);
                else y=tokdate.substr(7,4);
                record.push_back(symbol);
                record.push_back(_market);
                record.push_back("D");
                record.push_back(y+m+d);

            }
            else
            {
                LOG_WARN("bad date "<<tokdate);
                continue;
            }
        //LOG_DEBUG(tokdate);

            bool isn;
            t1=t3;
            t2=res.find(tkvalstart,t1);
            t3=res.find(tkend,t2+1);
            string tokopen=Trim(res.substr(t2+tkvalstart.size(),t3-t2-tkvalstart.size()-1));
            isn=true;
            if (tokopen!=string("-"))
            {
                try
                {
                    std::stod(tokopen);
                }
                catch (...)
                {
                    isn=false;
                    LOG_WARN("wrong number "<<tokopen);
                }
            }
            //LOG_DEBUG(tokopen);
            if (!isn) continue;
            t1=t3;
            t2=res.find(tkvalstart,t1);
            t3=res.find(tkend,t2+1);
            string tokhigh=Trim(res.substr(t2+tkvalstart.size(),t3-t2-tkvalstart.size()-1));
            isn=true;
            if (tokhigh!=string("-"))
            {
                try
                {
                    std::stod(tokhigh);
                }
                catch (...)
                {
                    isn=false;
                    LOG_WARN("wrong number "<<tokhigh);
                }
            }
          //  LOG_DEBUG(tokhigh);
            if (!isn) continue;

            t1=t3;
            t2=res.find(tkvalstart,t1);
            t3=res.find(tkend,t2+1);
            string toklow=Trim(res.substr(t2+tkvalstart.size(),t3-t2-tkvalstart.size()-1));
            isn=true;
            if (toklow!=string("-"))
            {
                try
                {
                    std::stod(toklow);
                }
                catch (...)
                {
                    isn=false;
                    LOG_WARN("wrong number "<<toklow);
                }
            }
            //LOG_DEBUG(toklow);
            if (!isn) continue;

            t1=t3;
            t2=res.find(tkvalstart,t1);
            t3=res.find(tkend,t2+1);
            string tokclose=Trim(res.substr(t2+tkvalstart.size(),t3-t2-tkvalstart.size()-1));
            isn=true;
            try
            {
                std::stod(tokclose);
            }
            catch (...)
            {
                isn=false;
                LOG_WARN("wrong number "<<tokclose);
            }
            //LOG_DEBUG(tokclose);
            if (!isn) continue;
            if (tokopen=="-") tokopen=tokclose;
            if (tokhigh=="-") tokhigh=tokclose;
            if (toklow=="-") toklow=tokclose;
            t1=t3;
            t2=res.find(tkvolstart,t1);
            t3=res.find(tkend,t2+1);
            string tokvolume=Trim(res.substr(t2+tkvolstart.size(),t3-t2-tkvolstart.size()-1));
            tokvolume.erase(std::remove(tokvolume.begin(), tokvolume.end(), ','), tokvolume.end());//remove ','

            isn=true;
            try
            {
                std::stod(tokvolume);
            }
            catch (...)
            {
                isn=false;
                LOG_WARN("wrong number "<<tokvolume);
            }
            //LOG_DEBUG(tokvolume);
            if (!isn) continue;

            record.push_back(tokopen);
            record.push_back(tokhigh);
            record.push_back(toklow);
            record.push_back(tokclose);
            record.push_back(tokvolume);
            record.push_back("0.0");//open interest =0
            values.push_back(record);
            if (Trim(res.substr(t3,8))=="</table>")
            cicle=false;
        }
        start+=200;
    }
    return values;

}

}
