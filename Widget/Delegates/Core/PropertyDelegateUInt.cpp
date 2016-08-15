/*
   Copyright (c) 2012-1015 Alex Zhondin <qtinuum.team@gmail.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "PropertyDelegateUInt.h"
#include "../../../Core/Core/PropertyUInt.h"
#include "../PropertyDelegateFactory.h"
#include "../PropertyEditorHandler.h"

#include <QSpinBox>
#include <QLineEdit>
#include <QLocale>

const quint32 qtn_u_2 = std::numeric_limits<quint32>::max() / 2 + 1;
static qint32 qtn_u2i(quint32 val)
{
    return qint32(val - qtn_u_2);
}
static quint32 qtn_i2u(qint32 val)
{
    return (quint32)val + qtn_u_2;
}

class SpinBoxUnsigned: public QSpinBox
{
public:
	SpinBoxUnsigned(QWidget* parent)
		: QSpinBox(parent)
	{
	}

	QLineEdit *lineEdit() const { return QSpinBox::lineEdit(); }

protected:
	int valueFromText(const QString& text) const override
	{
		return qtn_u2i(locale().toUInt(text));
	}

	QString textFromValue(int val) const override
	{
		return locale().toString(qtn_i2u(val)).replace(QRegExp("\\s*"), "");
	}
};

class QtnPropertyUIntSpinBoxHandler: public QtnPropertyEditorHandler<QtnPropertyUIntBase, SpinBoxUnsigned>
{
public:
	QtnPropertyUIntSpinBoxHandler(QtnPropertyUIntBase &property,
								  SpinBoxUnsigned &editor);

private:
	virtual void updateEditor() override;

	void onTextEdited(const QString &text);
	void onValueChanged(int value);
	void onEditingFinished();

	unsigned block;
};

static bool regUIntDelegate = QtnPropertyDelegateFactory::staticInstance()
                                .registerDelegateDefault(&QtnPropertyUIntBase::staticMetaObject
                                , &qtnCreateDelegate<QtnPropertyDelegateUInt, QtnPropertyUIntBase>
                                , "SpinBox");

QWidget* QtnPropertyDelegateUInt::createValueEditorImpl(QWidget* parent, const QRect& rect, QtnInplaceInfo* inplaceInfo)
{
	auto spinBox = new SpinBoxUnsigned(parent);
    spinBox->setGeometry(rect);

    new QtnPropertyUIntSpinBoxHandler(owner(), *spinBox);

    if (inplaceInfo)
    {
        spinBox->selectAll();
    }

    return spinBox;
}

bool QtnPropertyDelegateUInt::propertyValueToStr(QString& strValue) const
{
	strValue = QLocale().toString(owner().value());
	return true;
}

QtnPropertyUIntSpinBoxHandler::QtnPropertyUIntSpinBoxHandler(QtnPropertyUIntBase &property, SpinBoxUnsigned &editor)
	: QtnPropertyEditorHandlerType(property, editor)
	, block(0)
{
	if (!property.isEditableByUser())
		editor.setReadOnly(true);

	editor.setRange(qtn_u2i(property.minValue()), qtn_u2i(property.maxValue()));
	editor.setSingleStep(qtn_u2i(property.stepValue()));

	updateEditor();

	editor.setKeyboardTracking(false);
	editor.installEventFilter(this);
	QObject::connect(&editor,
					 static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
					 this,
					 &QtnPropertyUIntSpinBoxHandler::onValueChanged);
	QObject::connect(editor.lineEdit(),
					 &QLineEdit::textEdited,
					 this,
					 &QtnPropertyUIntSpinBoxHandler::onTextEdited);
	QObject::connect(&editor,
					 &QSpinBox::editingFinished,
					 this,
					 &QtnPropertyUIntSpinBoxHandler::onEditingFinished);
}

void QtnPropertyUIntSpinBoxHandler::updateEditor()
{
	block++;

	if (property().valueIsHidden())
		editor().setValue(0);
	else
		editor().setValue(qtn_u2i(property().value()));

	block--;
}

void QtnPropertyUIntSpinBoxHandler::onTextEdited(const QString &text)
{
	bool ok = false;
	auto value = text.toUInt(&ok);

	if (ok)
	{
		block++;
		editor().setValue(qtn_u2i(value));
		block--;
	}
}

void QtnPropertyUIntSpinBoxHandler::onValueChanged(int value)
{
	if (block > 0)
		return;

	property().edit(qtn_i2u(value));
}

void QtnPropertyUIntSpinBoxHandler::onEditingFinished()
{
	if (canApply())
		property().edit(qtn_i2u(editor().value()));

	applyReset();
}