#include "wndtimeline.h"
#include "ui_wndtimeline.h"

#include "frmconfig.h"
#include "Config.h"

#include <QMessageBox>
#include <QLabel>
#include <QCommandLinkButton>
#include <QFormLayout>

//TODO: pass to config
#define MSG_COUNT 50

WndTimeline::WndTimeline(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::WndTimeline)
{
    ui->setupUi(this);

	connect(ui->btnTimeline, SIGNAL(clicked()), this, SLOT(onTimeline()));
	connect(ui->btnMentions, SIGNAL(clicked()), this, SLOT(onMentions()));
	connect(ui->btnDirects, SIGNAL(clicked()), this, SLOT(onDirect()));
	connect(&_twitter, SIGNAL(onFriendsTimeline(Timeline *, int)), this, SLOT(onFriendsTimeline(Timeline *, int)));
	connect(&_twitter, SIGNAL(onFriendPicture(QTwitPicture)), this, SLOT(onFriendPicture(QTwitPicture)));

	_credentials.loadConfig();

	//If proxy
	_twitter.setProxy( CONFIG.getProxyUsername(), CONFIG.getProxyPassword() );
	_twitter.setTwitterAccount( _credentials.getUsername(), _credentials.getPassword() );

	_tipoReq = _TIPO_NADA;
}

WndTimeline::~WndTimeline()
{
	delete ui;
}

void WndTimeline::onTimeline()
{
	if( !_checkCredentials() ) return;
	_tipoReq = _TIPO_TIMELINE;
	_setWaiting(true);
	_twitter.getFriendsTimeline(MSG_COUNT);
}

void WndTimeline::onMentions()
{
	if( !_checkCredentials() ) return;
	_tipoReq = _TIPO_TIMELINE;
	_setWaiting(true);
	_twitter.getMentionsTimeline(MSG_COUNT);
}

void WndTimeline::onDirect()
{
	if( !_checkCredentials() ) return;
	_tipoReq = _TIPO_DIRECT;
	_setWaiting(true);
	_twitter.getDirectsTimeline(MSG_COUNT);
}

void WndTimeline::_setWaiting(bool waiting)
{
	if( waiting ) {
		ui->statusBar->showMessage("Pegando mensagens");
                ui->scrTimeline->setEnabled(false);
	} else {
		ui->statusBar->clearMessage();
		ui->scrTimeline->setEnabled(true);
	}
}

void WndTimeline::onFriendsTimeline(Timeline *timeline, int error)
{
	if( error != 0 ) return;

	for(int i=0; i < timeline->getCount(); i++) {
		const QString &text = timeline->getParam(i, "text");
		const QString &user = timeline->getParam(i,_tipoReq==_TIPO_TIMELINE?"user_screen_name":"sender_screen_name");
		const QString &id = timeline->getParam(i,"id");
		const QString &picUrl = timeline->getParam(i,"user_profile_image_url");

		if( (i+1) > _frameList.size() ) {
			_createItem(i, id, user, picUrl, text);
		} else {
			_updateItem(i, id, user, picUrl, text);

		}

	}
	_setWaiting(false);
}

void WndTimeline::_createItem(int pos, const QString &id, const QString &user, const QString &picUrl, const QString &text)
{
	QFrame *fra = new QFrame(ui->scrTimeline);
	_frameList.push_back(fra);
	QVBoxLayout *mainLay = new QVBoxLayout();
	fra->setLayout( mainLay );
	fra->layout()->setMargin(1);
	fra->layout()->setSizeConstraint(QLayout::SetMinimumSize);
	fra->setStyleSheet("background-color: rgb(234, 255, 234);");
	ui->layTimeline->addWidget( fra );

	QFormLayout *layText = new QFormLayout( fra );

	//Text ---------------------------------------------------------
	QLabel *lbl = new QLabel(ui->scrTimeline);
	lbl->setObjectName("lblText");

	QString itemText = "<a href=\"@" + user + "\"><font color='green'>" + user + "</font></a> ";
	itemText.append( _changeLinks(text) );
	itemText.append("<BR>");
	itemText.append("<a href=\"@@" + id);
	itemText.append("\">Reply</a> - <a href=\"##" + QString::number(pos) + "\">Retweet</a>");
	lbl->setText( itemText );

	QLabel *lblImg = new QLabel(ui->scrTimeline);
	lblImg->setObjectName("lblImg");
	lblImg->setToolTip(user);
	lblImg->setBackgroundRole(QPalette::Base);
	//lblImg->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	lblImg->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	lblImg->setScaledContents(true);
	lblImg->resize(48, 48);

	//TODO: caso NULL, colocar imagem vazia.
	const QImage *img = _getPicture(user, picUrl);
	if( img != NULL ) {
		lblImg->setPixmap( QPixmap::fromImage(*img) );
		lblImg->resize(lblImg->pixmap()->size());
	}

	connect(lbl, SIGNAL(linkActivated(QString)), this, SLOT(linkClicked(QString)));
	lbl->setWordWrap(true);

	//QSpacerItem *s1 = new QSpacerItem(48, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);
	//QSpacerItem *s2 = new QSpacerItem(10, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);
	//layText->addItem(s1);
	layText->addRow(lblImg, lbl);

	mainLay->addLayout(layText);

	//Line ---------------------------------------------------------
	QFrame *line = new QFrame(fra);
	line->setObjectName(QString::fromUtf8("line"));
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Plain);
	fra->layout()->addWidget(line);
}

void WndTimeline::_updateItem(int pos, const QString &id, const QString &user, const QString &picUrl, const QString &text)
{
	QFrame *fra = _frameList.at(pos);
	const QObjectList &list = fra->children();

	const QImage *img = _getPicture(user, picUrl);

	for( QObjectList::const_iterator it = list.constBegin(); it != list.constEnd(); it++ ) {
		QObject* obj = *it;
		if( obj->objectName() == "lblText" ) {
			QLabel *lbl = (QLabel *)obj;
			lbl->setText( "<a href=\"@" + user + "\"><font color='green'>" + user + "</font></a> " +
						  _changeLinks(text) );
		} else if( obj->objectName() == "lblImg" ) {
			if( img != NULL ) {
				QLabel *lblImg = (QLabel *)obj;
				lblImg->setPixmap( QPixmap::fromImage( img->scaled(48, 48, Qt::IgnoreAspectRatio) ) );
				//QSize size = lblImg->pixmap()->size();
				QSize size;
				size.setHeight(48); size.setWidth(48);

				lblImg->resize(size);
			}
		}
	}
}

const QImage *WndTimeline::_getPicture(const QString &user, const QString &picUrl)
{
	return _twitter.getPicture(user, picUrl);
}

void WndTimeline::linkClicked(QString desc)
{
	QMessageBox::information(this, "link", desc);
}

QString WndTimeline::_changeLinks(const QString &text)
{
	QString ret;
	int ini = 0;
	int end = 0;

	while( ini != -1 ) {
		ini = text.indexOf("http://", end);
		int ini1 = text.indexOf("@", end);
		int ini2 = text.indexOf("#", end);
		if( ini1 != -1 || ini2 != -1 ) {
			int first = ini1;
			if( (ini2 != -1 && ini2 < ini1) || ini1 == -1 ) first = ini2;
			if( first != -1 && first+1 <= text.size() ) {
				if( !text.at(first+1).isLetterOrNumber() )
					first = -1;
			}
			ini1 = first;
		}

		if( (ini1 != -1 && ini1 < ini) || ini == -1 ) ini = ini1;

		if( ini != -1 ) {
			if( ini > 0 ) ret += text.mid(end, ini-end);
			end = _endTok(text, ini);
			QString link = text.mid(ini, end-ini);
			ret += "<a href=\"" + link + "\">" + link + "</a>";
		}
	}

	if( ini != text.size() && end != -1 ) {
		ret += text.mid(end);
	}
	return ret;
}

int WndTimeline::_endTok(const QString &text, int pos)
{
	int i;
	for(i=pos; i < text.size(); i++) {
		QChar c = text.at(i);
		if( !(c.isLetterOrNumber() || c == '/' || c == ':' || c == '?' ||
			  c == '&' || c == '-' || c == '_' || c == '.' || c == '@' || c == '#' ) ) {
			return i;
		}
	}
	return i;
}

bool WndTimeline::_checkCredentials()
{
	if( !_credentials.hasUserSet() ) {
		QMessageBox::critical(this, tr("Erro logando"), tr("O usuário e senha do twitter<BR>nÃ£o estÃ£o configurados"));
		return false;
	}
	return true;
}

void WndTimeline::onFriendPicture(const QTwitPicture &pic)
{
	qDebug() << "CHEGOU IMG " << pic.getUsername();
	QList< QFrame* >::iterator itf = _frameList.begin();

	for(; itf != _frameList.end(); itf++) {
		QFrame *fra = *itf;

		const QObjectList &list = fra->children();
		for( QObjectList::const_iterator it = list.constBegin(); it != list.constEnd(); it++ ) {
			QObject* obj = *it;
			if( obj->objectName() == "lblImg" ) {
				QLabel *lblImg = (QLabel *)obj;
				QString user = (QString)lblImg->toolTip();
				if( user == pic.getUsername() ) {
					lblImg->setPixmap( QPixmap::fromImage(pic) );
					lblImg->resize(48, 48);
					lblImg->setScaledContents(true);
				}
			}
		}
	}//framelist
}

void WndTimeline::on_actionConfigurar_triggered()
{
	FrmConfig cfg(this);
	cfg.setModal(true);
	cfg.exec();
	_credentials.loadConfig();
}

void WndTimeline::on_actionSair_triggered()
{
	this->close();
}
