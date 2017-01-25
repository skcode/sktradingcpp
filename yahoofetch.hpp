#ifndef YAHOOFETCH_HPP_INCLUDED
#define YAHOOFETCH_HPP_INCLUDED

#include <string>
#include <map>
#include <vector>
namespace yahoofetch {

    /**
        scarica i dati da yahoo come vettore di vettori nel formato date (YYYY-MM-DD),open,high,low,close,volume,adjclose
    */
 const std::vector< std::vector<std::string> > quotes(std::string symbol) ;

}

#endif // YAHOOFETCH_HPP_INCLUDED
