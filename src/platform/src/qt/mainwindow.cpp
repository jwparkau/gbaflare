#include <platform/qt/mainwindow.h>
#include <platform/common/platform.h>
#include <gba/emulator.h>
#include "./ui_mainwindow.h"

#include <QImage>
#include <QPixmap>
#include <QKeyEvent>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	ui->graphicsView->installEventFilter(this);

	scene = new QGraphicsScene(this);
	scene->setBackgroundBrush(Qt::black);
	ui->graphicsView->setScene(scene);
	render(shared.framebuffer);

	QAudioFormat format;
	format.setSampleRate(SAMPLE_RATE);
	format.setChannelCount(2);
	format.setByteOrder(QAudioFormat::Endian::LittleEndian);
	format.setSampleType(QAudioFormat::SignedInt);
	format.setSampleSize(16);
	format.setCodec("audio/pcm");

	QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
	if (!info.isFormatSupported(format)) {
		fprintf(stderr, "audio error\n");
	}

	audio = new QAudioOutput(format, this);
#ifdef __unix__
	audio->setBufferSize(SAMPLES_PER_FRAME * 2 + 200);
#else
	audio->setBufferSize(12000);
#endif
	qbuffer = audio->start();

	emu_thread = new EmulatorThread;
	connect(emu_thread, SIGNAL(endOfFrame()), SLOT(onEndOfFrame()));
	connect(emu_thread, SIGNAL(signalUI(float)), SLOT(onSignalUI(float)));
}

MainWindow::~MainWindow()
{
	emu_cnt.emulator_running = false;
	emu_thread->wait();

	delete emu_thread;
	delete audio;
	delete scene;
	delete ui;
}

void MainWindow::scale_view()
{
	ui->graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::render(u16 *pixels)
{
	ui->graphicsView->scene()->clear();
	QImage image = QImage((unsigned char*)pixels, LCD_WIDTH, LCD_HEIGHT, QImage::Format_RGB555).rgbSwapped();
	QPixmap pixmap = QPixmap::fromImage(image);
	ui->graphicsView->scene()->addPixmap(pixmap);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
	QMainWindow::resizeEvent(event);
	scale_view();
}

void MainWindow::onEndOfFrame()
{
	if (!emu_cnt.emulator_running) {
		return;
	}

	int state = emu_cnt.emulator_state;
	shared.lock.lock();
	render(shared.framebuffer);
	if (state == EMULATION_RUNNING && emu_cnt.throttle_enabled) {
		qbuffer->write((const char *)shared.audiobuffer, SAMPLES_PER_FRAME * sizeof(*shared.audiobuffer));
	}
	shared.lock.unlock();
};

void MainWindow::onSignalUI(float fps)
{
	int state = emu_cnt.emulator_state;
	ui->actionReset->setEnabled(state == EMULATION_RUNNING || state == EMULATION_PAUSED);

	ui->actionPause->setEnabled(state == EMULATION_RUNNING || state == EMULATION_PAUSED);
	ui->actionPause->setChecked(state == EMULATION_PAUSED);

	ui->actionStop->setEnabled(state == EMULATION_RUNNING || state == EMULATION_PAUSED);

	ui->actionFast_Forward->setEnabled(state == EMULATION_RUNNING || state == EMULATION_PAUSED);
	ui->actionFast_Forward->setChecked(!emu_cnt.throttle_enabled);

	ui->actionPrint_FPS->setEnabled(state == EMULATION_RUNNING);
	ui->actionPrint_FPS->setChecked(emu_cnt.print_fps);

	if (state == EMULATION_RUNNING) {
		QString status = QString("FPS: %1").arg(fps);
		ui->statusbar->showMessage(status, 500);
	}
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
	int key = event->key();

	joypad_buttons button = translate_key(key);
	if (button == NOT_MAPPED) {
		QMainWindow::keyPressEvent(event);
	} else {
		update_joypad(button, true);
	}
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
	int key = event->key();

	switch (key) {
		case Qt::Key_0:
			emu_cnt.toggle_throttle();
			return;
		case Qt::Key_P:
			emu_cnt.toggle_print_fps();
			return;
		case Qt::Key_Escape:
			emu_cnt.request_pause = true;
			return;
		case Qt::Key_F5:
			emu_cnt.request_reset = true;
			return;
		case Qt::Key_F12:
			emu_cnt.request_close = true;
			return;
		case Qt::Key_F10:
			emu_cnt.debug = !emu_cnt.debug;
			return;
	}

	joypad_buttons button = translate_key(key);
	if (button == NOT_MAPPED) {
		QMainWindow::keyReleaseEvent(event);
	} else {
		update_joypad(button, false);
	}
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
	QKeyEvent *keyEvent = nullptr;

	if (event->type() == QEvent::KeyPress) {
		keyEvent = dynamic_cast<QKeyEvent *>(event);
		this->keyPressEvent(keyEvent);
		return true;
	} else if (event->type() == QEvent::KeyRelease) {
		keyEvent = dynamic_cast<QKeyEvent *>(event);
		this->keyReleaseEvent(keyEvent);
		return true;
	}

	return false;
}

joypad_buttons MainWindow::translate_key(int key)
{
	switch (key) {
		case Qt::Key_Right:
			return BUTTON_RIGHT;
		case Qt::Key_Left:
			return BUTTON_LEFT;
		case Qt::Key_Up:
			return BUTTON_UP;
		case Qt::Key_Down:
			return BUTTON_DOWN;
		case Qt::Key_X:
			return BUTTON_A;
		case Qt::Key_Z:
			return BUTTON_B;
		case Qt::Key_1:
		case Qt::Key_Return:
			return BUTTON_START;
		case Qt::Key_2:
		case Qt::Key_Backspace:
			return BUTTON_SELECT;
		case Qt::Key_S:
			return BUTTON_R;
		case Qt::Key_A:
			return BUTTON_L;
		default:
			return NOT_MAPPED;
	}
}

void MainWindow::onToolbarOpenROM()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Choose ROM file"), QDir::homePath(), tr("ROM (*.gba)"));
	if (!filename.isEmpty()) {
		shared.lock.lock();
		shared.cartridge_filename = filename.toStdString();
		shared.lock.unlock();
		emu_cnt.request_open = true;
	}
}

void MainWindow::onToolbarQuitApplication()
{
	QApplication::quit();
}

void MainWindow::onToolbarPrintFPS(bool checked)
{
	emu_cnt.print_fps = checked;
}

void MainWindow::onToolbarStop()
{
	emu_cnt.request_close = true;
}

void MainWindow::onToolbarPause()
{
	emu_cnt.request_pause = true;
}

void MainWindow::onToolbarReset()
{
	emu_cnt.request_reset = true;
}

void MainWindow::onToolbarFastForward(bool checked)
{
	emu_cnt.throttle_enabled = !checked;
}

void EmulatorThread::run()
{
	auto tick_start = std::chrono::steady_clock::now();
	std::chrono::duration<double> frame_duration(1.0 / FPS);

	for (;;) {
		if (!emu_cnt.emulator_running) {
			break;
		}

		if (emu_cnt.emulator_state == EMULATION_PAUSED) {
			//emit endOfFrame();
			emu_cnt.process_events();
		} else if (emu_cnt.emulator_state == EMULATION_RUNNING) {
			emu.run_one_frame();
			shared.lock.lock();
			std::memcpy(shared.framebuffer, framebuffer, FRAMEBUFFER_SIZE * sizeof(*framebuffer));
			std::memcpy(shared.audiobuffer, audiobuffer, AUDIOBUFFER_SIZE * sizeof(*audiobuffer));
			shared.lock.unlock();
			//emit endOfFrame();
			emu_cnt.process_events();
		} else {
			shared.lock.lock();
			ZERO_ARR(shared.framebuffer);
			ZERO_ARR(shared.audiobuffer);
			shared.lock.unlock();
			//emit endOfFrame();
			emu_cnt.process_events();
		}

		std::chrono::duration<double> sec = std::chrono::steady_clock::now() - tick_start;
		double fps = 1 / sec.count();
		if (emu_cnt.print_fps) {
			fprintf(stderr, "fps: %f\n", fps);
		}

		if (emu_cnt.throttle_enabled || emu_cnt.emulator_state != EMULATION_RUNNING) {
			while (std::chrono::steady_clock::now() - tick_start < frame_duration)
				;
		}
		tick_start = std::chrono::steady_clock::now();

		emit endOfFrame();
		emit signalUI(fps);
	}

	emu.quit();
}
