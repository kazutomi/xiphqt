from turbogears import controllers, expose, flash, validate
# from model import *
from turbogears import identity, redirect
from cherrypy import request, response
from icebreaker import json

import logging
log = logging.getLogger("icebreaker.controllers")

class Root(controllers.RootController):
    @expose(template="icebreaker.templates.icebreaker")
    # @identity.require(identity.in_group("admin"))
    def index(self):
        import time
        return dict(now=time.ctime(), greeting="poooooooooooooooop")

    @expose(template="icebreaker.templates.login")
    def login(self, forward_url=None, previous_url=None, *args, **kw):

        if not identity.current.anonymous \
            and identity.was_login_attempted() \
            and not identity.get_identity_errors():
            raise redirect(forward_url)

        forward_url=None
        previous_url= request.path

        if identity.was_login_attempted():
            msg=_("The credentials you supplied were not correct or "
                   "did not grant access to this resource.")
        elif identity.get_identity_errors():
            msg=_("You must provide your credentials before accessing "
                   "this resource.")
        else:
            msg=_("Please log in.")
            forward_url= request.headers.get("Referer", "/")
            
        response.status=403
        return dict(message=msg, previous_url=previous_url, logging_in=True,
                    original_parameters=request.params,
                    forward_url=forward_url)

    @expose()
    def logout(self):
        identity.current.logout()
        raise redirect("/")

    @expose(template="icebreaker.templates.list")
    def list(self):
        from model import data
        return dict(data=data.select())

    @expose(template="icebreaker.templates.create")
    def create(self):
        from model import data
        from selectshuttle.widgets import SelectShuttle
        from turbogears.widgets import TableForm
        return dict(data=data.select())
        form_name="foo"
        validating_form = widgets.Form(fields=[create])
        full_class_name = "selectshuttle.SelectShuttle"
        create = SelectShuttle(
        name="select_shuttle_demo",
        label = "The shuttle",
        title_available = "Available options",
        title_selected = "Selected options",
        # All data should be provided as a list of tuples, in the form of
        # ("id", "value"). ATM, id should be an int
        available_options = [(i, "Option %d"%i) for i in xrange(5)],
        default = dict(selected=[(i, "Option %d"%i) for i in xrange(3)])
        )

    @expose()
    def post_data(self, **kw):
        kw = self.validating_form.validate(kw)
        return "<b>Coerced data:</b><br />%s<br /><br /> " % kw
