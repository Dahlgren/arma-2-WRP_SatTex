#include "widget.h"
#include "ui_widget.h"
#include <QtWidgets>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
	ui->retranslateUi(this);
	break;
    default:
	break;
    }
}


void Widget::Read_Arguments(int argc, char *argv[])
{
	// 1 wrp
	// 2 ofp_sources
	// 3 satellite_resolution
	// 4 satellite
	qDebug() << "arg1: " << argv[1] << "arg2: " << argv[2] << "arg3: " << argv[3] << "arg4: " << argv[4];;

	char wrpname[255], ofpsources[255], satbmp[255];
	strcpy (wrpname, argv[1]);
	strcpy (ofpsources, argv[2]);
	QString str = argv[3];
	bool ok;
	int satres = str.toInt(&ok, 10);
	strcpy (satbmp, argv[4]);

	// launch the program

	// read the WRP file
	ReadFile(wrpname);

	// write text log file of texture names
	Write_Texture_Names(argv[4]);

	// clean out textures for easier use in main routine
	Prepare_TextStrings();

	// load OFP PNG textures and merge to large image
	Create_Image(ofpsources, satres, satbmp);
}


int Widget::ReadFile(char *wrpname)
{
	FILE *map;
	char sig[33];
	int x = 0, z = 0, px = 0, pz = 0;
	TexIndex = 0;

	map = fopen (wrpname, "rb");
	if (!map)
	{
		QMessageBox::information(this, tr("Unable to open file"), tr("sorry could not find/read 4WVR %1 file. exiting...").arg(wrpname));
		exit (1);
	}

	fread (sig, 4, 1, map);
	sig[4] = 0;
	fread(&MapSize, 4, 1, map);
	fread(&MapSize, 4, 1, map);

	// read elevations
	pz = MapSize;
	for (int zx = 0; zx < MapSize * MapSize; zx++)
	{
		fread(&wrp[x][z].Elevation,sizeof(wrp[x][z].Elevation),1,map);

		// wrp coordinates
		x++;
		if (x == MapSize)
		{
			z++; x = 0;
		}
		if (z == MapSize)
		{
			z = 0;
		}
	}

	// read textures IDs
	x = 0, z = 0, TexIndex = 0;
	px = 0, pz = MapSize;

	for (int tx = 0; tx < MapSize * MapSize; tx++)
	{
		wrp[x][z].TexIndex = 0;
		fread(&wrp[x][z].TexIndex,sizeof(wrp[x][z].TexIndex),1,map);

		// wrp coordinates
		x++;
		if (x == MapSize)
		{
			z++; x = 0;
		}
		if (z == MapSize)
		{
			z = 0;
		}
	}

	//textures 32 char length and total of 512
	for (int ix = 0; ix < 512; ix++)
	{
		sig[0] = 0;
		fread(sig, 32, 1, map);
		strcpy (TexStrings[ix], sig);
	}

	fclose(map);

	return MapSize;
}


// write all WRP texture paths + file names into a text log file
void Widget::Write_Texture_Names(QString fileName)
{
	fileName.append("_textures_log.txt");
	QFile logFile(fileName);
	logFile.open(QIODevice::WriteOnly);
	QTextStream stream(&logFile);

	stream << "Index, Texture Name" << endl;

	for (int ix = 0; ix < 512; ix++)
	{
		// if we have texture, put in some debug texts.
		if (strlen(TexStrings[ix]) > 0)
		{
			stream << ix << ", " << TexStrings[ix] << endl;
		}
	}

	logFile.close();
}


void Widget::Prepare_TextStrings()
{
	for (int ix = 0; ix < 512; ix++)
	{
		// cut the .EXT from the file name string.
		TexStrings[ix][strlen(TexStrings[ix])-4]=0;
		strlwr(TexStrings[ix]);

		// if we have texture, put in some debug texts.
		if (strlen(TexStrings[ix]) > 0)
		{
			qDebug() << "index: " << ix << " Texture: " << TexStrings[ix];
		}
	}
}


void Widget::Create_Image(char *RootOfSources, int resolution, char *satellite)
{
	// version number
	const QString version = "v0.4";

	int res_x = 0, res_y = 0, texRes = round (resolution / MapSize);

	// ugly temporary filename.
	int x = 0, z = 0;
	char filename[255];

	qDebug() << "Resolution: " << resolution << " texRes: " << texRes;

	// resultImage is the satellite image, the big one.
    // Format_RGB16 : 16-bit RGB format (5-6-5).
    // Format_RGB555 : 16-bit RGB format (5-5-5). The unused most significant bit is always zero.
    QImage *resultImage = new QImage(resolution, resolution, QImage::Format_RGB555);
	// error check for memory allocation
	if (resultImage == NULL)
	{
		// error :)
	}
	resultImage->fill(qRgb(255,55,255));

	// create the satellite image and draw it (but not visible yet).
	QPainter painter;
	painter.begin(resultImage);
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	painter.drawImage(0, 0, *resultImage);

	// loadable image, our texture.
	QImage* images[512];	// Our texture image cache/table
	QRectF target(0, 0, texRes, texRes);
	// this needs to be the real OFP PAC/PAA texture resolution (I think?) :)
	QRectF source(0, 0, 512, 512);

	QProgressDialog progressDialog(this);
	progressDialog.setRange(0, (MapSize * MapSize));
	progressDialog.setWindowTitle(tr("WRP to Satellite %1").arg(version));

	// Make sure all the QImage pointers are NULL before we begin
	// (since we haven't loaded any of them yet)
	for (int i=0; i < 512; i++) images[i] = NULL;

	for (int tx = 0; tx < MapSize * MapSize; tx++)
	{
		TexIndex = wrp[x][z].TexIndex;

		// Has this particular texture been loaded yet?
		if (images[TexIndex]==NULL)
		{
			// Nope - let's load it
			sprintf (filename, "%s%s.png", RootOfSources, TexStrings[TexIndex]);
			images[TexIndex] = new QImage(filename, 0);
			// TODO: Add error handling here - if the operation above returns NULL,
			// the image loading failed
			QFile logFile("WRP_sattex_debug_log.txt");
			logFile.open(QIODevice::Append);
			QTextStream stream(&logFile);
			if (images[TexIndex]->isNull())
			{
				stream << "ERROR! loading: " << filename << ", Height: " << images[TexIndex]->height() << ", Width: " << images[TexIndex]->width() << ", Depth: " << images[TexIndex]->depth() << endl;
				logFile.close();
			}
			else
			{
				stream << "Loaded: " << filename << ", Height: " << images[TexIndex]->height() << ", Width: " << images[TexIndex]->width() << ", Depth: " << images[TexIndex]->depth() << endl;
				logFile.close();
			}
		}

		target.setRect(res_x, res_y, texRes, texRes);
		painter.drawImage(target, *images[TexIndex], source);

		// this works except its inverted.
		// bitmap coordinates
		res_x += texRes;
		if (res_x == resolution)
		{
			res_y += texRes; res_x = 0;
		}
		if (res_y == resolution)
		{
			res_y = 0;
		}

		// wrp coordinates
		x++;
		if (x == MapSize)
		{
			z++; x = 0;
		}
		if (z == MapSize)
		{
			z = 0;
		}

		progressDialog.setValue(tx);
		progressDialog.setLabelText(tr("Merging OFP image from WRP %3 of %4\nCell [%1, %2]")
					    .arg(x).arg(z).arg(tx).arg(MapSize * MapSize));
		qApp->processEvents();
	}

	painter.end();

	// delete memory for images[]
	for (int i=0; i < 512; i++)
	{
		if (images[i] != NULL) delete images[i];
	};

	resultImage->save(satellite, 0, -1);
	delete resultImage;
	//QMessageBox::warning(this, tr("WRP to Satellite %1").arg(version), tr("%1 has been saved.\n\nYou are free to exit the tool.\nThanks for using.").arg(satellite));
	qDebug() << "saved satellite image!";
}
