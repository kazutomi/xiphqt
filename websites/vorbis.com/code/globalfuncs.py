# globalfuncs.py
#
# miscellaneous global functions

import string

def t2h(s):
    codes = [ ['&', '&amp;'], ['\n', '<br>'] ]
    for c in codes:
        s = string.replace(s, c[0], c[1])
    return s

def dbesc(s):
    return string.replace(s, "'", "''")


def get_about_box():
    from infobox import InfoBox
    box = InfoBox()

    box.title = 'What is Ogg Vorbis?'
    box.more = '/faq.psp'
    box.title_image = '/images/NewsBox-bg.png'
    box.title_image_r = '/images/NewsBox-right-bg.png'
    box.fontsize = -1

    one = box.start()
    two = """
    Ogg Vorbis is a completely open, patent-free, professional audio encoding and streaming technology with all the benefits of Open Source.
    <br><br>
    <center>
    Learn more by reading the <a href="/faq.psp">FAQ</a>
    &nbsp;&nbsp;
    --
    &nbsp;&nbsp;
    Get <a href="/faq.psp">set up</a> to listen to Vorbis.
    </center>
    """

    three = box.finish()

    return one + two + three


def get_feature_box():
    from infobox import InfoBox
    box = InfoBox()

    box.title = 'Donate!'
    box.title_image = '/images/NewsBox-bg.png'
    box.title_image_r = '/images/NewsBox-right-bg.png'
    box.fontsize = -1

    one = box.start()
    
    two = """
    The <a href="http://www.xiph.org">Xiph.org Foundation</a> is accepting <a href="http://www.xiph.org/ogg/vorbis/donate.html">donations</a> via Paypal. By supporting Xiph.org, you help fund the development of free, open multimedia fomats and tools. To send money simply click on the icon below.<br><p>
    <center><a href="https://secure.paypal.com/xclick/business=donate@xiph.org&item_name=Donation&item_number=DONATE&return=http://www.vorbis.com"><img src="http://www.paypal.com/images/lgo/logo3.gif"></a></center>
    <p>You can also visit our <a href="http://www.cafeshops.com/oggwear">Cafeshop</a> for the coolest Ogg gear.
    """

    three = box.finish()

    return one + two + three
