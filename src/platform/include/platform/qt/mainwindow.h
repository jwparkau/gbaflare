#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <common/types.h>
#include <platform/common/platform.h>
#include <QMainWindow>
#include <QGraphicsScene>
#include <QThread>
#include <QBuffer>
#include <QAudioOutput>
#include <QString>
#include <QMutex>
#include <string>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class EmulatorThread : public QThread
{
	Q_OBJECT
	public:
		void run();

	signals:
		void endOfFrame();
		void signalUI(float fps);
};

class MainWindow : public QMainWindow
{
	Q_OBJECT

	public:
		QGraphicsScene *scene{};
		EmulatorThread *emu_thread{};
		QIODevice *qbuffer{};
		QAudioOutput *audio{};
		bool audio_paused{};
		int do_scale{};
		MainWindow(QWidget *parent = nullptr);
		~MainWindow();

		void render(u16 *pixels);
		void keyPressEvent(QKeyEvent *event);
		void keyReleaseEvent(QKeyEvent *event);
		joypad_buttons translate_key(int key);
		bool eventFilter(QObject *object, QEvent *event);
		void scale_view();

	private:
		Ui::MainWindow *ui;
		void resizeEvent(QResizeEvent *event);

	public slots:
		void onEndOfFrame();
		void onToolbarOpenROM();
		void onToolbarPrintFPS(bool checked);
		void onToolbarStop();
		void onToolbarReset();
		void onToolbarPause();
		void onToolbarFastForward(bool checked);
		void onSignalUI(float fps);
		void onToolbarQuitApplication();
};


#endif // MAINWINDOW_H
