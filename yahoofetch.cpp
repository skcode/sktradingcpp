#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <map>
#include "logger.hpp"
#include "netutils.hpp"
#include "yahoofetch.hpp"

using namespace std;
namespace yahoofetch {






const std::string fetchquotes(std::string symbol) {
	std::string buffer;
	//ip - 66.196.66.213
	std::string baseurl = "http://ichart.finance.yahoo.com/table.csv?";
	//std::string baseurl = "http://66.196.66.213/table.csv?";
	baseurl = baseurl + "s=" + symbol + "&ignore=.csv";
	buffer=netutils::httpfetch(baseurl);
	return buffer;
}


const std::vector< std::vector<std::string> > quotes(std::string symbol ) {
    std::vector< std::vector<std::string> > v;
    std::string _symbol=symbol;
/*    if (market=="MLSE")  _symbol=symbol+".MI";
    else  if (market=="NYSE")  _symbol=symbol;
    else  if (market=="NASDAQ")  _symbol=symbol;
    else THROW_EXCEPTION("unknown market "<<market);*/
    std::istringstream iss(fetchquotes(_symbol));
    std::string line;
	int linenum = 0;

    while (getline(iss, line)) {
      //  LOG_DEBUG(line);
		if (linenum > 1) {//skip header  (date,open,high,low,close,volume,adjclose)


            std::vector<std::string> vec;
            std::istringstream buf(line);

            for(std::string token; getline(buf, token, ','); )
                vec.push_back(token);
            if (vec.size()==7) v.push_back(vec);
            //for (auto &s : vc) std::cout << s<<"\t";
        }
        linenum++;
    }
    return v;
}





}
