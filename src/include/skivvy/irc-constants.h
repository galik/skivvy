#pragma once
#ifndef _SOOKEE_IRC_CONSTANTS_H_
#define _SOOKEE_IRC_CONSTANTS_H_

#include <skivvy/types.h>

namespace skivvy { namespace irc {

using namespace skivvy::types;

#define KEYWORD(w) const str w = #w

// Create const str versions of
// various IRC key command words
KEYWORD(PASS);
KEYWORD(NICK);
KEYWORD(USER);
KEYWORD(JOIN);
KEYWORD(KICK);
KEYWORD(PART);
KEYWORD(PING);
KEYWORD(PONG);
KEYWORD(PRIVMSG);
KEYWORD(ACTION);
KEYWORD(QUERY);
KEYWORD(MODE);


const str RPL_WELCOME = "001";
const str RPL_YOURHOST = "002";
const str RPL_CREATED = "003";
const str RPL_MYINFO = "004";
const str RPL_BOUNCE = "005";
const str RPL_TRACELINK = "200";
const str RPL_TRACECONNECTING = "201";
const str RPL_TRACEHANDSHAKE = "202";
const str RPL_TRACEUNKNOWN = "203";
const str RPL_TRACEOPERATOR = "204";
const str RPL_TRACEUSER = "205";
const str RPL_TRACESERVER = "206";
const str RPL_TRACESERVICE = "207";
const str RPL_TRACENEWTYPE = "208";
const str RPL_TRACECLASS = "209";
const str RPL_TRACERECONNECT = "210";
const str RPL_STATSLINKINFO = "211";
const str RPL_STATSCOMMANDS = "212";
const str RPL_STATSCLINE = "213";
const str RPL_STATSNLINE = "214";
const str RPL_STATSILINE = "215";
const str RPL_STATSKLINE = "216";
const str RPL_STATSQLINE = "217";
const str RPL_STATSYLINE = "218";
const str RPL_ENDOFSTATS = "219";
const str RPL_UMODEIS = "221";
const str RPL_SERVICEINFO = "231";
const str RPL_ENDOFSERVICES = "232";
const str RPL_SERVICE = "233";
const str RPL_SERVLIST = "234";
const str RPL_SERVLISTEND = "235";
const str RPL_STATSVLINE = "240";
const str RPL_STATSLLINE = "241";
const str RPL_STATSUPTIME = "242";
const str RPL_STATSOLINE = "243";
const str RPL_STATSHLINE = "244";
const str RPL_STATSSLINE = "244";
const str RPL_STATSPING = "246";
const str RPL_STATSBLINE = "247";
const str RPL_STATSDLINE = "250";
const str RPL_LUSERCLIENT = "251";
const str RPL_LUSEROP = "252";
const str RPL_LUSERUNKNOWN = "253";
const str RPL_LUSERCHANNELS = "254";
const str RPL_LUSERME = "255";
const str RPL_ADMINME = "256";
const str RPL_ADMINLOC1 = "257";
const str RPL_ADMINLOC2 = "258";
const str RPL_ADMINEMAIL = "259";
const str RPL_TRACELOG = "261";
const str RPL_TRACEEND = "262";
const str RPL_TRYAGAIN = "263";
const str RPL_NONE = "300";
const str RPL_AWAY = "301";
const str RPL_USERHOST = "302";
const str RPL_ISON = "303";
const str RPL_UNAWAY = "305";
const str RPL_NOWAWAY = "306";
const str RPL_WHOISUSER = "311";
const str RPL_WHOISSERVER = "312";
const str RPL_WHOISOPERATOR = "313";
const str RPL_WHOWASUSER = "314";
const str RPL_ENDOFWHO = "315";
const str RPL_WHOISCHANOP = "316";
const str RPL_WHOISIDLE = "317";
const str RPL_ENDOFWHOIS = "318";
const str RPL_WHOISCHANNELS = "319";
const str RPL_LISTSTART = "321";
const str RPL_LIST = "322";
const str RPL_LISTEND = "323";
const str RPL_CHANNELMODEIS = "324";
const str RPL_UNIQOPIS = "325";
const str RPL_NOTOPIC = "331";
const str RPL_TOPIC = "332";
const str RPL_INVITING = "341";
const str RPL_SUMMONING = "342";
const str RPL_INVITELIST = "346";
const str RPL_ENDOFINVITELIST = "347";
const str RPL_EXCEPTLIST = "348";
const str RPL_ENDOFEXCEPTLIST = "349";
const str RPL_VERSION = "351";
const str RPL_WHOREPLY = "352";
const str RPL_NAMREPLY = "353";
const str RPL_KILLDONE = "361";
const str RPL_CLOSING = "362";
const str RPL_CLOSEEND = "363";
const str RPL_LINKS = "364";
const str RPL_ENDOFLINKS = "365";
const str RPL_ENDOFNAMES = "366";
const str RPL_BANLIST = "367";
const str RPL_ENDOFBANLIST = "368";
const str RPL_ENDOFWHOWAS = "369";
const str RPL_INFO = "371";
const str RPL_MOTD = "372";
const str RPL_INFOSTART = "373";
const str RPL_ENDOFINFO = "374";
const str RPL_MOTDSTART = "375";
const str RPL_ENDOFMOTD = "376";
const str RPL_YOUREOPER = "381";
const str RPL_REHASHING = "382";
const str RPL_YOURESERVICE = "383";
const str RPL_MYPORTIS = "384";
const str RPL_TIME = "391";
const str RPL_USERSSTART = "392";
const str RPL_USERS = "393";
const str RPL_ENDOFUSERS = "394";
const str RPL_NOUSERS = "395";
const str ERR_NOSUCHNICK = "401";
const str ERR_NOSUCHSERVER = "402";
const str ERR_NOSUCHCHANNEL = "403";
const str ERR_CANNOTSENDTOCHAN = "404";
const str ERR_TOOMANYCHANNELS = "405";
const str ERR_WASNOSUCHNICK = "406";
const str ERR_TOOMANYTARGETS = "407";
const str ERR_NOSUCHSERVICE = "408";
const str ERR_NOORIGIN = "409";
const str ERR_NORECIPIENT = "411";
const str ERR_NOTEXTTOSEND = "412";
const str ERR_NOTOPLEVEL = "413";
const str ERR_WILDTOPLEVEL = "414";
const str ERR_BADMASK = "415";
const str ERR_UNKNOWNCOMMAND = "421";
const str ERR_NOMOTD = "422";
const str ERR_NOADMININFO = "423";
const str ERR_FILEERROR = "424";
const str ERR_NONICKNAMEGIVEN = "431";
const str ERR_ERRONEUSNICKNAME = "432";
const str ERR_NICKNAMEINUSE = "433";
const str ERR_NICKCOLLISION = "436";
const str ERR_UNAVAILRESOURCE = "437";
const str ERR_USERNOTINCHANNEL = "441";
const str ERR_NOTONCHANNEL = "442";
const str ERR_USERONCHANNEL = "443";
const str ERR_NOLOGIN = "444";
const str ERR_SUMMONDISABLED = "445";
const str ERR_USERSDISABLED = "446";
const str ERR_NOTREGISTERED = "451";
const str ERR_NEEDMOREPARAMS = "461";
const str ERR_ALREADYREGISTRED = "462";
const str ERR_NOPERMFORHOST = "463";
const str ERR_PASSWDMISMATCH = "464";
const str ERR_YOUREBANNEDCREEP = "465";
const str ERR_YOUWILLBEBANNED = "466";
const str ERR_KEYSET = "467";
const str ERR_CHANNELISFULL = "471";
const str ERR_UNKNOWNMODE = "472";
const str ERR_INVITEONLYCHAN = "473";
const str ERR_BANNEDFROMCHAN = "474";
const str ERR_BADCHANNELKEY = "475";
const str ERR_BADCHANMASK = "476";
const str ERR_NOCHANMODES = "477";
const str ERR_BANLISTFULL = "478";
const str ERR_NOPRIVILEGES = "481";
const str ERR_CHANOPRIVSNEEDED = "482";
const str ERR_CANTKILLSERVER = "483";
const str ERR_RESTRICTED = "484";
const str ERR_UNIQOPPRIVSNEEDED = "485";
const str ERR_NOOPERHOST = "491";
const str ERR_NOSERVICEHOST = "492";
const str ERR_UMODEUNKNOWNFLAG = "501";
const str ERR_USERSDONTMATCH = "502";

}} // sookee::irc

#endif // _SOOKEE_IRC_CONSTANTS_H_
