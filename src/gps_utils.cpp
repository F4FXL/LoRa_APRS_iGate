#include "configuration.h"
#include "gps_utils.h"

extern Configuration  Config;
extern WiFi_AP        *currentWiFi;
extern int            stationMode;

namespace GPS_Utils {

String double2string(double n, int ndec) {
    String r = "";
    int v = n;
    r += v;
    r += '.';
    int i;
    for (i=0;i<ndec;i++) {
        n -= v;
        n = 10 * abs(n);
        v = n;
        r += v;
    }
    return r;
}

String processLatitudeAPRS(double lat) {
  String degrees = double2string(lat,6);
  String north_south, latitude, convDeg3;
  float convDeg, convDeg2;

  if (abs(degrees.toFloat()) < 10) {
    latitude += "0";
  }
  Serial.println(latitude);
  if (degrees.indexOf("-") == 0) {
    north_south = "S";
    latitude += degrees.substring(1,degrees.indexOf("."));
  } else {
    north_south = "N";
    latitude += degrees.substring(0,degrees.indexOf("."));
  }
  convDeg = abs(degrees.toFloat()) - abs(int(degrees.toFloat()));
  convDeg2 = (convDeg * 60)/100;
  convDeg3 = String(convDeg2,6);
  latitude += convDeg3.substring(convDeg3.indexOf(".")+1,convDeg3.indexOf(".")+3) + "." + convDeg3.substring(convDeg3.indexOf(".")+3,convDeg3.indexOf(".")+5);
  latitude += north_south;
  return latitude;
}

String processLongitudeAPRS(double lon) {
  String degrees = double2string(lon,6);
  String east_west, longitude, convDeg3;
  float convDeg, convDeg2;
  
  if (abs(degrees.toFloat()) < 100) {
    longitude += "0";
  }
  if (abs(degrees.toFloat()) < 10) {
    longitude += "0";
  }
  if (degrees.indexOf("-") == 0) {
    east_west = "W";
    longitude += degrees.substring(1,degrees.indexOf("."));
  } else {
    east_west = "E";
    longitude += degrees.substring(0,degrees.indexOf("."));
  }
  convDeg = abs(degrees.toFloat()) - abs(int(degrees.toFloat()));
  convDeg2 = (convDeg * 60)/100;
  convDeg3 = String(convDeg2,6);
  longitude += convDeg3.substring(convDeg3.indexOf(".")+1,convDeg3.indexOf(".")+3) + "." + convDeg3.substring(convDeg3.indexOf(".")+3,convDeg3.indexOf(".")+5);
  longitude += east_west;
  return longitude;
}

String generateBeacon() {
  String stationLatitude, stationLongitude, beaconPacket;
  if (stationMode==1 || stationMode==2) {
    stationLatitude = processLatitudeAPRS(currentWiFi->latitude);
    stationLongitude = processLongitudeAPRS(currentWiFi->longitude);
    beaconPacket = Config.callsign + ">APLRG1,qAC:=" + stationLatitude + "L" + stationLongitude;
    if (stationMode == 1) {
      beaconPacket += "&";
    } else {
      beaconPacket += "a";
    }
    beaconPacket += Config.iGateComment;
  } else { //stationMode 3 y 4
    stationLatitude = processLatitudeAPRS(Config.digi.latitude);
    stationLongitude = processLongitudeAPRS(Config.digi.longitude);
    beaconPacket = Config.callsign + ">APLRG1:=" + stationLatitude + "L" + stationLongitude + "#" + Config.digi.comment;
  }
  return beaconPacket;
}

}