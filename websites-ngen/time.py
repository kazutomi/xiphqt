import time
import datetime

USUAL_TIME_FORMAT = "%Y-%m-%dT%H:%M:%SZ"
dt = datetime.datetime.utcnow()
print "It is currently %s in W3C Date-Time-ese. Have a nice day." % dt.strftime(USUAL_TIME_FORMAT)
