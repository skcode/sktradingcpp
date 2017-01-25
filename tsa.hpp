/* 
 * File:   tsa.hpp
 * Author: sk
 *
 * Created on 23 ottobre 2015, 19.57
 */
#include <string>
#include "security.h"

#ifndef TSA_HPP
#define	TSA_HPP
using namespace std;
class tsa {
public:
    tsa(string db){
       security sp500=security(db,"^GSPC","",timeseries::freq_enum::daily);
       security dax=security(db,"^GDAXI","",timeseries::freq_enum::daily);
       security vix=security(db,"^VIX","",timeseries::freq_enum::daily);
       
       
    
    }
    tsa(const tsa& orig);
    virtual ~tsa();
private:

};

#endif	/* TSA_HPP */

