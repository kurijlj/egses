import matplotlib.colors as colors
import matplotlib.cm as cm
import numpy as np


class IndexTracker(object):
    """Abstract base class for IndexTracker classes.
    """

    def __init__(self, ax, sm):
        pass

    def onscroll(self, event):
        pass

    def _update(self):
        pass


class SlicesTracker(IndexTracker):
    def __init__(self, ax, sm):
        self.ax = ax

        self.sm = sm
        rows, cols, self.slices = sm.shape
        self.ind = self.slices//2

        self.im = ax.imshow(self.sm[:, :, self.ind])
        self.update()

    def onscroll(self, event):
        print("%s %s" % (event.button, event.step))
        if event.button == 'up':
            self.ind = (self.ind + 1) % self.slices
        else:
            self.ind = (self.ind - 1) % self.slices
        self.update()

    def _update(self):
        self.im.set_data(self.sm[:, :, self.ind])
        self.ax.set_ylabel('slice %s' % self.ind)
        self.im.axes.figure.canvas.draw()


class FusedSlicesTracker(IndexTracker):
    def __init__(self, ax, smX, smY):
        self.ax = ax

        self.smX = smX
        self.smY = smY
        # Put extent assertion here!
        rows, cols, self.slices = smX.shape
        self.ind = self.slices//2

        self.imX = ax.imshow(self.smX[:, :, self.ind], cmap=cm.gray)
        self.imY = ax.imshow(
                self.smY[:, :, self.ind],
                cmap=cm.viridis,
                interpolation="bilinear",
                alpha=0.6)
        self._update()

    def onscroll(self, event):
        print("%s %s" % (event.button, event.step))
        if event.button == 'up':
            self.ind = (self.ind + 1) % self.slices
        else:
            self.ind = (self.ind - 1) % self.slices
        self._update()

    def _update(self):
        self.imX.set_data(self.smX[:, :, self.ind])
        self.imY.set_data(self.smY[:, :, self.ind])
        self.ax.set_ylabel('slice %s' % self.ind)
        self.imX.axes.figure.canvas.draw()
        self.imY.axes.figure.canvas.draw()


class IsoSlicesTracker(IndexTracker):
    def __init__(self, ax, smX, smY):
        self.ax = ax

        self.smX = smX
        self.smY = smY
        # Put extent assertion here!
        rows, cols, self.slices = smX.shape
        self.ind = self.slices//2

        self.imX = ax.imshow(self.smX[:, :, self.ind], cmap=cm.gray)
        normalize = colors.Normalize(
                self.smY[:, :, self.ind].min(),
                self.smY[:, :, self.ind].max())
        self.imY = ax.imshow(
                self.smY[:, :, self.ind],
                cmap=cm.spectral,
                interpolation="bilinear",
#                norm=normalize,
                vmin=0.0,
                vmax=1.0,
                alpha=0.6)
        self.levels = np.arange(0.1, smY[:, :, self.ind].max(), 0.1)
        self.CS = ax.contour(
                self.smY[:, :, self.ind],
                self.levels,
                linewidths=0.3,
                cmap=cm.spectral)
        self.clabels = ax.clabel(self.CS, self.levels)
        self._update()

    def onscroll(self, event):
        print("%s %s" % (event.button, event.step))
        if event.button == 'up':
            self.ind = (self.ind + 1) % self.slices
        else:
            self.ind = (self.ind - 1) % self.slices
        self._update()

    def _update(self):
        self.ax.cla()
        self.ax.imshow(self.smX[:, :, self.ind], cmap=cm.gray)
        normalize = colors.Normalize(
                self.smY[:, :, self.ind].min(),
                self.smY[:, :, self.ind].max())
        self.ax.imshow(
                self.smY[:, :, self.ind],
#                cmap=cm.viridis,
                cmap=cm.spectral,
                interpolation="bilinear",
#                norm=normalize,
                vmin=0.0,
                vmax=1.0,
                alpha=0.6)
#        self.imX.set_data(self.smX[:, :, self.ind])
#        self.imY.set_data(self.smY[:, :, self.ind])
        self.levels = np.arange(0.1, self.smY[:, :, self.ind].max(), 0.1)
#        for coll in self.CS.collections:
#            coll.remove()
#        for label in self.clabels:
#            print(label)
        self.CS = self.ax.contour(
                self.smY[:, :, self.ind],
                self.levels,
                linewidths=0.3,
                cmap=cm.spectral)
        self.ax.clabel(self.CS, self.levels)
        self.ax.set_ylabel('slice %s' % self.ind)
        self.imX.axes.figure.canvas.draw()
        self.imY.axes.figure.canvas.draw()
