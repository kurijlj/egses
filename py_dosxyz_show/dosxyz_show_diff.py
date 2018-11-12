#!/usr/bin/env python3
# =============================================================================
# <one line to give the program's name and a brief idea of what it does.>
#
#  Copyright (C) <yyyy> <Author Name> <author@mail.com>
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option)
# any later version.
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <http://www.gnu.org/licenses/>.
#
# =============================================================================


# =============================================================================
#
# <Put documentation here>
#
# <yyyy>-<mm>-<dd> <Author Name> <author@mail.com>
#
# * <programfilename>.py: created.
#
# =============================================================================


import argparse
import matplotlib
import numpy as np
import tkinter as tk
import egsdosetools as edt
import matplotlib.cm as cm
import matplotlib.pyplot as plt
from enum import Enum
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.backends.backend_tkagg import NavigationToolbar2TkAgg

matplotlib.use("TkAgg")


# =============================================================================
# Utility classes and functions
# =============================================================================

class DisplayPlane(Enum):
    """Class to wrap up enumerated values that describe plane of 3D space to
    be displayed.
    """

    xz = 0
    yz = 1
    xy = 2


class ProgramAction(object):
    """Abstract base class for all program actions, that provides execute.

    The execute method contains code that will actually be executed after
    arguments parsing is finished. The method is called from within method
    run of the CommandLineApp instance.
    """

    def __init__(self, exitf):
        self._exit_app = exitf

    def execute(self):
        pass


def _format_epilog(epilogAddition, bugMail):
    """Formatter for generating help epilogue text. Help epilogue text is an
    additional description of the program that is displayed after the
    description of the arguments. Usually it consists only of line informing
    to which email address to report bugs to, or it can be completely
    omitted.

    Depending on provided parameters function will properly format epilogue
    text and return string containing formatted text. If none of the
    parameters are supplied the function will return None which is default
    value for epilog parameter when constructing parser object.
    """

    fmtMail = None
    fmtEpilog = None

    if epilogAddition is None and bugMail is None:
        return None

    if bugMail is not None:
        fmtMail = 'Report bugs to <{bugMail}>.'\
            .format(bugMail=bugMail)
    else:
        fmtMail = None

    if epilogAddition is None:
        fmtEpilog = fmtMail

    elif fmtMail is None:
        fmtEpilog = epilogAddition

    else:
        fmtEpilog = '{addition}\n\n{mail}'\
            .format(addition=epilogAddition, mail=fmtMail)

    return fmtEpilog


def _formulate_action(Action, **kwargs):
    """Factory method to create and return proper action object.
    """

    return Action(**kwargs)


def _is_phantom_file(filename):
    """Test if file is dosxyznrc phantom file, i.e. chechk if .egsphant
    suffix is present in the given filename.
    """

    # Check if filename is nonempty string.
    if not isinstance(filename, str) or not filename:
        return False

    ext = filename.split('.')[-1]

    if 'egsphant' == ext:
        return True
    else:
        return False


def _is_dose_file(filename):
    """Test if file is dosxyznrc dose file, i.e. chechk if .3ddose
    suffix is present in the given filename.
    """

    # Check if filename is nonempty string.
    if not isinstance(filename, str) or not filename:
        return False

    ext = filename.split('.')[-1]

    if '3ddose' == ext:
        return True
    else:
        return False


# =============================================================================
# Command line app class
# =============================================================================

class CommandLineApp(object):
    """Actual command line app object containing all relevant application
    information (NAME, VERSION, DESCRIPTION, ...) and which instantiates
    action that will be executed depending on the user input from
    command line.
    """

    def __init__(
            self,
            programName=None,
            programDescription=None,
            programLicense=None,
            versionString=None,
            yearString=None,
            authorName=None,
            authorMail=None,
            epilog=None):

        self.programLicense = programLicense
        self.versionString = versionString
        self.yearString = yearString
        self.authorName = authorName
        self.authorMail = authorMail

        fmtEpilog = _format_epilog(epilog, authorMail)

        self._parser = argparse.ArgumentParser(
                prog=programName,
                description=programDescription,
                epilog=fmtEpilog,
                formatter_class=argparse.RawDescriptionHelpFormatter
            )

        # Since we add argument options to groups by calling group
        # method add_argument, we have to sore all that group objects
        # somewhere before adding arguments. Since we want to store all
        # application relevant data in our application object we use
        # this list for that purpose.
        self._argumentGroups = []

    @property
    def programName(self):
        """Utility function that makes accessing program name attribute
        neat and hides implementation details.
        """
        return self._parser.prog

    @property
    def programDescription(self):
        """Utility function that makes accessing program description
        attribute neat and hides implementation details.
        """
        return self._parser.description

    def add_argument_group(self, title=None, description=None):
        """Adds an argument group to application object.
        At least group title must be provided or method will rise
        NameError exception. This is to prevent creation of titleless
        and descriptionless argument groups. Although this is allowed bu
        argparse module I don't see any use of a such utility."""

        if title is None:
            raise NameError('Missing arguments group title.')

        group = self._parser.add_argument_group(title, description)
        self._argumentGroups.append(group)

        return group

    def _group_by_title(self, title):
        group = None

        for item in self._argumentGroups:
            if title == item.title:
                group = item
                break

        return group

    def add_argument(self, *args, **kwargs):
        """Wrapper for add_argument methods of argparse module. If
        parameter group is supplied with valid group name, argument will
        be added to that group. If group parameter is omitted argument
        will be added to parser object. In a case of invalid group name
        it rises ValueError exception.
        """

        if 'group' not in kwargs or kwargs['group'] is None:
            self._parser.add_argument(*args, **kwargs)

        else:
            group = self._group_by_title(kwargs['group'])

            if group is None:
                raise ValueError(
                        'Trying to reference nonexisten argument group.'
                    )

            else:
                kwargsr = {k: kwargs[k] for k in kwargs.keys() if 'group' != k}
                group.add_argument(*args, **kwargsr)

    def parse_args(self, args=None, namespace=None):
        """Wrapper for parse_args method of a parser object. It also
        instantiates action object that will be executed based on a
        input from command line.
        """

        arguments = self._parser.parse_args(args, namespace)

        if arguments.usage:
            self._action = _formulate_action(
                ProgramUsageAction,
                parser=self._parser,
                exitf=self._parser.exit)

        elif arguments.version:
            self._action = _formulate_action(
                ShowVersionAction,
                prog=self._parser.prog,
                ver=self.versionString,
                year=self.yearString,
                author=self.authorName,
                license=self.programLicense,
                exitf=self._parser.exit)

        else:
            filelist = (
                    arguments.phantomfile,
                    arguments.refdosefile,
                    arguments.difdosefile
                )
            self._action = _formulate_action(
                DefaultAction,
                prog=self._parser.prog,
                exitf=self._parser.exit,
                filelist=filelist)

    def run(self):
        """This method executes action code.
        """

        self._action.execute()


# =============================================================================
# GUI classes
# =============================================================================

class SliceTracker(object):
    """
    """

    def __init__(
            self,
            figure,
            ax,
            phantomdata,
            refdosedata,
            difdosedata,
            plane
    ):
        self._figure = figure
        self._ax = ax
        self._phantomdata = phantomdata
        self._plane = plane
        # self._showdose = tk.IntVar(0)
        self._showdose = False

        if difdosedata:
            self._dose = difdosedata.dose - refdosedata.dose
        else:
            self._dose = refdosedata.dose

        if DisplayPlane.xy == self._plane:
            self._slices = phantomdata.shape[0]

        elif DisplayPlane.yz == self._plane:
            self._slices = phantomdata.shape[1]

        else:
            self._slices = phantomdata.shape[2]

        self._index = self._slices // 2

        self.update()

    @property
    def voxsdens(self):
        return self._phantomdata.voxelsdensity

    @property
    def voxsdens_min(self):
        return self._phantomdata.voxelsdensity.min()

    @property
    def voxsdens_max(self):
        return self._phantomdata.voxelsdensity.max()

    @property
    def dose(self):
        return (self._dose / self._dose.max()) * 100.0

    @property
    def dose_min(self):
        return (self._dose.min() / self._dose.max()) * 100.0

    @property
    def dose_max(self):
        return 100.0

    @property
    def showdose(self):
        return self._showdose

    @showdose.setter
    def showdose(self, val):
        self._showdose = val

    # def on_show_dose(self):
    #     if 1 == self._showdose:
    #         self._showdose = 0
    #     else:
    #         self._showdose = 1

    #     self.update()

    def on_scroll(self, event):
        if event.button == 'up':
            self._index = (self._index + 1) % self._slices
        else:
            self._index = (self._index - 1) % self._slices
        self.update()

    def update(self):
        self._ax.clear()

        title = None
        density = None
        dose = None
        localdosemax = None

        if DisplayPlane.xy == self._plane:
            title = 'XY plane'
            density = self.voxsdens[self._index, :, :]
            if self._showdose:
                dose = self.dose[self._index, :, :]
                localdosemax = self.dose[self._index, :, :].max()

        elif DisplayPlane.yz == self._plane:
            title = 'YZ plane'
            density = self.voxsdens[:, self._index, :]
            if self._showdose:
                dose = self.dose[:, self._index, :]
                localdosemax = self.dose[:, self._index, :].max()

        else:
            title = 'XZ plane'
            density = self.voxsdens[:, :, self._index]
            if self._showdose:
                dose = self.dose[:, :, self._index]
                localdosemax = self.dose[:, :, self._index].max()

        self._ax.set_title(title)
        self._ax.set_xlabel('slice: {0}'.format(self._index))
        # self._ax.set_facecolor('gray')
        self._ax.imshow(
                density,
                cmap=cm.gray,
                vmin=self.voxsdens_min,
                vmax=self.voxsdens_max
            )

        if self._showdose:
            self._ax.imshow(
                    dose,
                    cmap=cm.spectral,
                    interpolation="bilinear",
                    vmin=self.dose_min,
                    vmax=self.dose_max,
                    alpha=0.6)

            levels = np.arange(10.0, localdosemax, 10.0)
            # contours = None
            if 0 != len(levels):
                contours = self._ax.contour(
                        dose,
                        levels,
                        linewidths=0.3,
                        cmap=cm.spectral)
                self._ax.clabel(contours, levels, fmt='%3d %%')

        self._figure.canvas.draw()


class SliceView(object):
    """
    """

    def __init__(self, frame, phantomdata, refdosedata, difdosedata, plane):

        # Initialize figure and canvas.
        self._figure = plt.Figure(figsize=(7, 5), dpi=72, tight_layout=True)
        # self._figure = plt.Figure(dpi=100)
        FigureCanvasTkAgg(self._figure, frame)
        self._figure.canvas.draw()
        # self._figure.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=1)
        self._figure.canvas.get_tk_widget().pack(side=tk.TOP, expand=False)

        # Initialize axes.
        self._axes = self._figure.add_subplot(1, 1, 1)

        # Set and initialize slice tracker.
        self._tracker = SliceTracker(
                self._figure,
                self._axes,
                phantomdata,
                refdosedata,
                difdosedata,
                plane
            )

        # Dosedata supplied, enable dose display.
        self._tracker.showdose = True

        # Enable tracker to respond to scroll event.
        self._figure.canvas.mpl_connect(
                'scroll_event',
                self._tracker.on_scroll
            )

        # Add toolbar to each view so user can zoom, take screenshots, etc.
        self._toolbar = NavigationToolbar2TkAgg(
                self._figure.canvas,
                frame
            )

        # Update toolbar display.
        self._toolbar.update()

    @property
    def showdose(self):
        return self._tracker.showdose

    @showdose.setter
    def showdose(self, val):
        self._tracker.showdose = val

    def update_view(self):
        self._tracker.update()


class DosXYZShowGUI(tk.Tk):
    """ A simple GUI application to show EGS phantom and 3ddose data.
    """

    def __init__(self, *args, **kwargs):

        # Initialize show dose switch.
        self._showdose = False

        # Remove phantomdata and dosedata from kwargs dict and pass
        # initialization data to parent class constructor.
        self._phantomdata = kwargs.pop('phantomdata')
        self._refdosedata = kwargs.pop('refdosedata')
        self._difdosedata = kwargs.pop('difdosedata')
        tk.Tk.__init__(self, *args, **kwargs)

        # Set app icon, window title and make window nonresizable.
        # tk.Tk.iconbitmap(self, default='dosxyz_show.ico')
        self.title('dosxyz_show.py')
        self.resizable(False, False)

        # Set topmost frames and align them to grid.
        self._sliceframe = tk.Frame(self)
        self._sliceframe.grid(
                column=0,
                row=0,
                columnspan=2,
                sticky=tk.N+tk.E+tk.S+tk.W
            )
        self._commandframe = tk.LabelFrame(self, text='Controls')
        self._commandframe.grid(column=1, row=1, sticky=tk.N+tk.E+tk.S+tk.W)

        # Set each of slice frames.
        self._framexz = tk.LabelFrame(self._sliceframe, text='XZ Plane')
        self._framexz.grid(column=0, row=0, sticky=tk.N+tk.E+tk.S+tk.W)
        self._frameyz = tk.LabelFrame(self._sliceframe, text='YZ Plane')
        self._frameyz.grid(column=1, row=0, sticky=tk.N+tk.E+tk.S+tk.W)
        self._framexy = tk.LabelFrame(self._sliceframe, text='XY Plane')
        self._framexy.grid(column=2, row=0, sticky=tk.N+tk.E+tk.S+tk.W)

        # Set each of views.
        self._viewxz = SliceView(
                self._framexz,
                self._phantomdata,
                self._refdosedata,
                self._difdosedata,
                DisplayPlane.xz
            )
        self._viewyz = SliceView(
                self._frameyz,
                self._phantomdata,
                self._refdosedata,
                self._difdosedata,
                DisplayPlane.yz
            )
        self._viewxy = SliceView(
                self._framexy,
                self._phantomdata,
                self._refdosedata,
                self._difdosedata,
                DisplayPlane.xy
            )

        # Set display and window controls. Default state for show dose check
        # button is enabled.
        state = 'normal'
        self._showdose = True

        # Initialize check button.
        self._showdosecheck = tk.Checkbutton(
                self._commandframe,
                text='Show dose',
                state=state,
                command=self.on_show_dose_check
            )

        # By default check button is selected dose data provided or not.
        self._showdosecheck.select()

        # Align check button to grid.
        self._showdosecheck.grid(column=0, row=0, sticky=tk.N+tk.E+tk.S+tk.W)

        # Initialize app quit button.
        button = tk.Button(
                self._commandframe,
                text='Quit',
                command=self.destroy
            )

        # Align button to grid.
        button.grid(column=0, row=1, sticky=tk.N+tk.E+tk.S+tk.W)

        # Update display.
        self.update()

    def on_show_dose_check(self):
        if self._showdose:
            self._showdose = False
            self._viewxz.showdose = False
            self._viewyz.showdose = False
            self._viewxy.showdose = False

        else:
            self._showdose = True
            self._viewxz.showdose = True
            self._viewyz.showdose = True
            self._viewxy.showdose = True

        self.update()

    def update(self):
        self._viewxz.update_view()
        self._viewyz.update_view()
        self._viewxy.update_view()


# =============================================================================
# App action classes
# =============================================================================

class ProgramUsageAction(ProgramAction):
    """Program action that formats and displays usage message to the stdout.
    """

    def __init__(self, parser, exitf):
        self._usageMessage = \
                '{usage}Try \'{prog} --help\' for more information.'\
                .format(usage=parser.format_usage(), prog=parser.prog)
        self._exit_app = exitf

    def execute(self):
        print(self._usageMessage)
        self._exit_app()


class ShowVersionAction(ProgramAction):
    """Program action that formats and displays program version information
    to the stdout.
    """

    def __init__(self, prog, ver, year, author, license, exitf):
        self._versionMessage = \
                '{0} {1} Copyright (C) {2} {3}\n{4}'\
                .format(prog, ver, year, author, license)
        self._exit_app = exitf

    def execute(self):
        print(self._versionMessage)
        self._exit_app()


class DefaultAction(ProgramAction):
    """Program action that wraps some specific code to be executed based on
    command line input. In this particular case it prints simple message
    to the stdout.
    """

    def __init__(self, prog, exitf, filelist):
        self._programName = prog
        self._exit_app = exitf
        self._filelist = filelist
        self._phantomdata = None
        self._refdosedata = None
        self._difdosedata = None

    def execute(self):
        # Do some basic sanity checks first.
        if not _is_phantom_file(self._filelist[0]):
            print(
                    '{0}: File \'{1}\' is not proper phantom file.'
                    .format(self._programName, self._filelist[0])
                )

            self._exit_app()

        if not _is_dose_file(self._filelist[1]):
            print(
                    '{0}: File \'{1}\' is not proper dose file.'
                    .format(self._programName, self._filelist[1])
                )

            self._exit_app()

        if self._filelist[2] is not None:
            if not _is_dose_file(self._filelist[2]):
                print(
                        '{0}: File \'{1}\' is not proper dose file.'
                        .format(self._programName, self._filelist[2])
                    )

                self._exit_app()

        # We have a proper phantom and reference dose file. Load the data.
        self._phantomdata = edt.xyzcls.PhantomFile(self._filelist[0])
        self._refdosedata = edt.xyzcls.DoseFile(self._filelist[1])

        # Another sanity check. Number of segments along axes for two files
        # must coincide.
        if self._phantomdata.shape != self._refdosedata.shape:
            print('{0}: (ERROR) Number of segments for phantom data and\
 reference dose data must coincide!'.format(self._programName))
            self._exit_app()

        if self._filelist[2] is not None:
            # We have proper diference dose file. Load it too.
            self._difdosedata = edt.xyzcls.DoseFile(self._filelist[2])

            # Check size of the difference dose file too.
            if self._phantomdata.shape != self._difdosedata.shape:
                print('{0}: (ERROR) Number of segments for phantom data and\
 difference dose data must coincide!'.format(self._programName))
                self._exit_app()

        else:
            self._difdosedata = None

        gui = DosXYZShowGUI(
                phantomdata=self._phantomdata,
                refdosedata=self._refdosedata,
                difdosedata=self._difdosedata
            )
        gui.mainloop()
        self._exit_app()


# =============================================================================
# Script main body
# =============================================================================

if __name__ == '__main__':

    description = 'Framework for application development \
implementing argp option parsing engine.\n\n\
Mandatory arguments to long options are mandatory for \
short options too.'
    license = 'License GPLv3+: GNU GPL version 3 or later \
<http://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and \
redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.'

    program = CommandLineApp(
        programDescription=description.replace('\t', ''),
        programLicense=license.replace('\t', ''),
        versionString='i.i',
        yearString='yyyy',
        authorName='Author Name',
        authorMail='author@mail.com',
        epilog=None)

    program.add_argument_group('general options')
    program.add_argument(
            '-V',
            '--version',
            action='store_true',
            help='print program version',
            group='general options')
    program.add_argument(
            '--usage',
            action='store_true',
            help='give a short usage message')
    program.add_argument(
            'phantomfile',
            metavar='PHANTOMFILE',
            type=str,
            help='dosxyznrc .egsphant file')
    program.add_argument(
            'refdosefile',
            metavar='REFDOSEFILE',
            type=str,
            help='dosxyznrc reference .3ddose file')
    program.add_argument(
            'difdosefile',
            metavar='DIFDOSEFILE',
            type=str,
            nargs='?',
            help='dosxyznrc .3ddose file to compare to reference one')

    program.parse_args()
    program.run()
