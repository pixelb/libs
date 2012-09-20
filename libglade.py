#This requires pygtk2

import gettext
import gtk
import gtk.glade

class GladeWrapper:
    """
    Superclass for glade based applications. Just derive from this
    and your subclass should create methods whose names correspond to
    the signal handlers defined in the glade file. Any other attributes
    in your class will be safely ignored.

    This class will give you the ability to do:
        subclass_instance.GtkWindow.method(...)
        subclass_instance.widget_name...
    """
    def __init__(self, Filename, WindowName):
        #load glade file.
        self.widgets = gtk.glade.XML(Filename, WindowName, gettext.textdomain())
        self.GtkWindow = getattr(self, WindowName)

        instance_attributes = {}
        for attribute in dir(self.__class__):
            instance_attributes[attribute] = getattr(self, attribute)
        self.widgets.signal_autoconnect(instance_attributes)

    def __getattr__(self, attribute): #Called when no attribute in __dict__
        widget = self.widgets.get_widget(attribute)
        if widget is None:
            raise AttributeError("Widget [" + attribute + "] not found")
        self.__dict__[attribute] = widget #add reference to cache
        return widget
