﻿#include "Editor.h"
#include <QAbstractItemView>
#include <QDebug>
#include <QLabel>
#include <QPainter>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QTextBlock>

int PlainTextEdit::mTabSize = 4;
int PlainTextEdit::mIndentSize = 2;
PlainTextEdit::PlainTextEdit(QWidget* parent)
    : QPlainTextEdit(parent)
{
    setStyleSheet("QPlainTextEdit{"
                  "selection-background-color:#3399FF;"
                  "selection-color: white;"
                  "border: 1px solid gray;}");

    QTextDocument* pTextDocument = document();
    //文本距离上下左右的边距
    pTextDocument->setDocumentMargin(2);

    QFont font;
    font.setFamily("Courier New");
    font.setPointSizeF(13);
    setFont(font);

    //功能1：数字行号
    mpLineNumberArea = new LineNumberArea(this);
    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect, int)), this, SLOT(updateLineNumberArea(QRect, int)));
    updateLineNumberAreaWidth(0);
    setLineWrapMode(QPlainTextEdit::WidgetWidth); //设置换行模式

    //功能2：自动补全
    mpStandardItemModel = new QStandardItemModel(this);
    initCompleteModel();
    QSortFilterProxyModel* pSortFilterProxyModel = new QSortFilterProxyModel(this);
    pSortFilterProxyModel->setSourceModel(mpStandardItemModel);
    pSortFilterProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    pSortFilterProxyModel->sort(0, Qt::AscendingOrder);
    mpCompleter = new QCompleter(this);
    mpCompleter->setModel(pSortFilterProxyModel);
    mpCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    mpCompleter->setWrapAround(false);
    mpCompleter->setWidget(this);
    mpCompleter->setCompletionMode(QCompleter::PopupCompletion);
    connect(mpCompleter, SIGNAL(highlighted(QModelIndex)), this, SLOT(showCompletionItemToolTip(QModelIndex)));
    connect(mpCompleter, SIGNAL(activated(QModelIndex)), this, SLOT(insertCompletionItem(QModelIndex)));

    //===============
    connect(this, SIGNAL(undoAvailable(bool)), SLOT(setUndoAvailable(bool)));
    connect(this, SIGNAL(redoAvailable(bool)), SLOT(setRedoAvailable(bool)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(updateHighlights()));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(updateCursorPosition()));
    updateHighlights();

}

int PlainTextEdit::lineNumberAreaWidth()
{
    int digits = 2;
    int lines = document()->blockCount();
    int max = qMax(1, lines);
    while (max >= 100)
    {
        max /= 10;
        ++digits;
    }
    const QFontMetrics fm(document()->defaultFont());
    qDebug().nospace() << __FILE__ << "(" << __LINE__ << ")" << __FUNCTION__
                       << " -- document()->defaultFont() size = " << document()->defaultFont().pointSize();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    int space = fm.horizontalAdvance(QLatin1Char('9')) * digits;
#else  // QT_VERSION_CHECK
    int space = fm.width(QLatin1Char('9')) * digits;
#endif // QT_VERSION_CHECK
    space += 8;
    return space;
}

void PlainTextEdit::lineNumberAreaPaintEvent(QPaintEvent* event)
{
    QPainter painter(mpLineNumberArea);
//    painter.fillRect(event->rect(), QColor(240, 240, 240));
    QTextBlock block = firstVisibleBlock();
    int        blockNumber = block.blockNumber();
    //计算第一个文本块
    qreal top = blockBoundingGeometry(block).translated(contentOffset()).top();
    qreal              bottom = top;
    const int          lineNumbersWidth = mpLineNumberArea->width();
    const QFontMetrics fm(document()->defaultFont());
    QTextDocument* pTextDocument = document();
    while (block.isValid() && top <= event->rect().bottom())
    {
        top = bottom;
        const qreal height = blockBoundingRect(block).height();
        bottom = top + height;
        QTextBlock nextBlock = block.next();
        QTextBlock nextVisibleBlock = nextBlock;
        int        nextVisibleBlockNumber = blockNumber + 1;
        if (!nextVisibleBlock.isVisible())
        {
            nextVisibleBlock = pTextDocument->findBlockByLineNumber(nextVisibleBlock.firstLineNumber());
            nextVisibleBlockNumber = nextVisibleBlock.blockNumber();
        }
        if (bottom < event->rect().top())
        {
            block = nextVisibleBlock;
            blockNumber = nextVisibleBlockNumber;
            continue;
        }

        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number;
            number = QString::number(blockNumber + 1);
            if (blockNumber == textCursor().blockNumber())
            {
                painter.setPen(QColor(64, 64, 64));
            }
            else
            {
                painter.setPen(Qt::gray);
            }
            painter.setFont(document()->defaultFont());
            painter.drawText(0, top, lineNumbersWidth, fm.height(), Qt::AlignRight, number);
        }
        block = nextVisibleBlock;
        blockNumber = nextVisibleBlockNumber;
    }
}

void PlainTextEdit::lineNumberAreaMouseEvent(QMouseEvent* event) {}

void PlainTextEdit::updateLineNumberAreaWidth(int newBlockCount)
{
    Q_UNUSED(newBlockCount);
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void PlainTextEdit::updateLineNumberArea(const QRect& rect, int dy)
{
    if (dy)
    {
        // 行号窗体跟随文本窗体滚动
        mpLineNumberArea->scroll(0, dy);
    }
    else
    {
        //文本窗体长度变化，重绘更新
        mpLineNumberArea->update(0, rect.y(), mpLineNumberArea->width(), rect.height());
    }
}

void PlainTextEdit::showCompletionItemToolTip(const QModelIndex& index) {}

void PlainTextEdit::insertCompletionItem(const QModelIndex& index)
{

    QVariant      value = index.data(Qt::UserRole);
    CompleterItem completerItem = qvariant_cast<CompleterItem>(value);
    QString       selectiontext = completerItem.mSelect;
    QStringList   completionlength = completerItem.mValue.split("\n");
    QTextCursor   cursor = textCursor();
    cursor.beginEditBlock();
    cursor.setPosition(cursor.position(), QTextCursor::MoveAnchor);
    cursor.setPosition(cursor.position() - mpCompleter->completionPrefix().length(), QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.insertText(completionlength[0]);
//    cursor.insertHtml(QString(R"(<a href="https://www.runoob.com/">%1</a>)").arg(completionlength[0]));
    cursor.endEditBlock();
    setTextCursor(cursor);
}

void PlainTextEdit::updateHighlights()
{
    QList<QTextEdit::ExtraSelection> selection1;
    setExtraSelections(selection1);
    QList<QTextEdit::ExtraSelection> selections = extraSelections();
    QTextEdit::ExtraSelection        selection;
    QColor                           lineColor = QColor(232, 242, 254);
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    selections.append(selection);
    setExtraSelections(selections);
}

void PlainTextEdit::updateCursorPosition()
{
    ensureCursorVisible();
}

void PlainTextEdit::resizeEvent(QResizeEvent* pEvent)
{
    qDebug().nospace() << __FILE__ << "(" << __LINE__ << ")" << __FUNCTION__
                       << " -- document()->defaultFont() size = " << document()->defaultFont().pointSize();
    QPlainTextEdit::resizeEvent(pEvent);
    qDebug().nospace() << __FILE__ << "(" << __LINE__ << ")" << __FUNCTION__
                       << " -- document()->defaultFont() size = " << document()->defaultFont().pointSize();
    //使用文本的矩形，它是去掉了间隔的矩形
    QRect cr = contentsRect();
    qDebug().nospace() << __FILE__ << "(" << __LINE__ << ")" << __FUNCTION__ << " -- ";
    mpLineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void PlainTextEdit::keyPressEvent(QKeyEvent* pEvent)
{
    bool shiftModifier = pEvent->modifiers().testFlag(Qt::ShiftModifier);
    bool controlModifier = pEvent->modifiers().testFlag(Qt::ControlModifier);
    bool isCompleterShortcut = controlModifier && (pEvent->key() == Qt::Key_Space); // CTRL+space
    if (mpCompleter && mpCompleter->popup()->isVisible())                           //补全窗口存在
    {
        switch (pEvent->key())
        {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            pEvent->ignore(); //分发给 QComplete 处理
            qDebug().nospace() << __FILE__ << "(" << __LINE__ << ")" << __FUNCTION__ << " -- ";
            return;
        default:
            break;
        }
    }
    if (pEvent->key() == Qt::Key_Tab || pEvent->key() == Qt::Key_Backtab)
    {
        // 缩进或者
        indentOrUnindent(pEvent->key() == Qt::Key_Tab);
        return;
    }
    if (!mpCompleter || !isCompleterShortcut)
    {
        qDebug().nospace() << __FILE__ << "(" << __LINE__ << ")" << __FUNCTION__ << " -- pEvent = " << pEvent->key();
        QPlainTextEdit::keyPressEvent(pEvent);
    }
    static QString eow("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-="); //文本jiesu
    QString        completionPrefix = wordUnderCursor();
    int            lastDotIndex = completionPrefix.lastIndexOf('.');
    if (lastDotIndex != -1) //
    {
        completionPrefix = completionPrefix.right(completionPrefix.length() - lastDotIndex - 1);
    }
    if ((!isCompleterShortcut) && (pEvent->text().isEmpty() || completionPrefix.length() < 1 || eow.contains(pEvent->text().right(1))))
    {
        mpCompleter->popup()->hide();
        return;
    }
    if (completionPrefix != mpCompleter->completionPrefix())
    {
        mpCompleter->setCompletionPrefix(completionPrefix);
    }
    QRect cr = cursorRect();
    cr.setWidth(mpCompleter->popup()->sizeHintForColumn(0) + mpCompleter->popup()->verticalScrollBar()->sizeHint().width());
    mpCompleter->complete(cr);
    if (mpCompleter->popup()->selectionModel()->selection().empty())
    {
        mpCompleter->popup()->setCurrentIndex(mpCompleter->completionModel()->index(0, 0));
    }
}

void PlainTextEdit::wheelEvent(QWheelEvent* event)
{
    qDebug().nospace() << __FILE__ << "(" << __LINE__ << ")" << __FUNCTION__
                       << " -- document()->defaultFont() size = " << document()->defaultFont().pointSize();
    if (event->modifiers() & Qt::ControlModifier)
    {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
        if (event->angleDelta().x() > 0 || event->angleDelta().y() > 0)
        {
#else  // QT_VERSION_CHECK
        if (event->delta() > 0)
        {
#endif // QT_VERSION_CHECK
            zoomIn();
        }
        else
        {
            zoomOut();
        }
    }
    QPlainTextEdit::wheelEvent(event);
}

int PlainTextEdit::firstNonSpace(const QString& text)
{
    int i = 0;
    while (i < text.size())
    {
        if (!text.at(i).isSpace())
        {
            return i;
        }
        ++i;
    }
    return i;
}

int PlainTextEdit::lineIndentPosition(const QString &text)
{
    int i = 0;
    while (i < text.size())
    {
        if (!text.at(i).isSpace())
        {
            break;
        }
        ++i;
    }
    int column = columnAt(text, i);
    return i - (column % mIndentSize);
}

int PlainTextEdit::spacesLeftFromPosition(const QString& text, int position)
{
    int i = position;
    while (i > 0)
    {
        if (!text.at(i - 1).isSpace())
        {
            break;
        }
        --i;
    }
    return position - i;
}

int PlainTextEdit::columnAt(const QString& text, int position)
{
    int column = 0;
    for (int i = 0; i < position; ++i)
    {
        if (text.at(i) == QLatin1Char('\t'))
        {
            column = column - (column % mTabSize) + mTabSize;
        }
        else
        {
            ++column;
        }
    }
    return column;
}

int PlainTextEdit::indentedColumn(int column, bool doIndent)
{
    int aligned = (column / mIndentSize) * mIndentSize;
    if (doIndent)
    {
        return aligned + mIndentSize;
    }
    if (aligned < column)
    {
        return aligned;
    }
    return qMax(0, aligned - mIndentSize);
}

QString PlainTextEdit::indentationString(int startColumn, int targetColumn)
{
    targetColumn = qMax(startColumn, targetColumn);
    QString s;
    int     alignedStart = startColumn - (startColumn % mTabSize) + mTabSize;
    if (alignedStart > startColumn && alignedStart <= targetColumn)
    {
        s += QLatin1Char('\t');
        startColumn = alignedStart;
    }
    if (int columns = targetColumn - startColumn)
    {
        int tabs = columns / mTabSize;
        s += QString(tabs, QLatin1Char('\t'));
        s += QString(columns - tabs * mTabSize, QLatin1Char(' '));
    }
    return s;
}

bool PlainTextEdit::cursorIsAtBeginningOfLine(const QTextCursor &cursor)
{
    QString text = cursor.block().text();
    int     fns = firstNonSpace(text);
    return (cursor.position() - cursor.block().position() <= fns);
}

LineNumberArea* PlainTextEdit::getMpLineNumberArea() const
{
    return mpLineNumberArea;
}

void PlainTextEdit::insertCompleterTypes(QStringList types)
{
    for (int k = 0; k < types.size(); ++k)
    {
        QStandardItem* pStandardItem = new QStandardItem(types[k]);
        //        pStandardItem->setIcon(ResourceCache::getIcon(":/Resources/icons/completerType.svg"));
        pStandardItem->setData(QVariant::fromValue(CompleterItem(types[k], types[k], "")), Qt::UserRole);
        mpStandardItemModel->appendRow(pStandardItem);
    }
}

QStringList PlainTextEdit::getKeywords()
{
    QStringList keywordsList;
    keywordsList << "algorithm"
                 << "and"
                 << "annotation"
                 << "assert"
                 << "block"
                 << "break"
                 << "class"
                 << "connect"
                 << "connector"
                 << "constant"
                 << "constrainedby"
                 << "der"
                 << "discrete"
                 << "each"
                 << "else"
                 << "elseif"
                 << "elsewhen"
                 << "encapsulated"
                 << "end"
                 << "enumeration"
                 << "equation"
                 << "expandable"
                 << "extends"
                 << "external"
                 << "false"
                 << "final"
                 << "flow"
                 << "for"
                 << "function"
                 << "if"
                 << "import"
                 << "impure"
                 << "in"
                 << "initial"
                 << "inner"
                 << "input"
                 << "loop"
                 << "model"
                 << "not"
                 << "operator"
                 << "or"
                 << "outer"
                 << "output"
                 << "optimization"
                 << "package"
                 << "parameter"
                 << "partial"
                 << "protected"
                 << "public"
                 << "pure"
                 << "record"
                 << "redeclare"
                 << "replaceable"
                 << "return"
                 << "stream"
                 << "then"
                 << "true"
                 << "type"
                 << "when"
                 << "while"
                 << "within";
    return keywordsList;
}

QStringList PlainTextEdit::getTypes()
{
    QStringList typesList;
    typesList << "String"
              << "Integer"
              << "Boolean"
              << "Real";
    return typesList;
}

void PlainTextEdit::initCompleteModel()
{
    mpStandardItemModel->clear();
    QStringList types = PlainTextEdit::getTypes();
    insertCompleterTypes(types);
    QStringList keywords = PlainTextEdit::getKeywords();
    insertCompleterTypes(keywords);
}

void PlainTextEdit::resetZoom()
{
    qDebug().nospace() << __FILE__ << "(" << __LINE__ << ")" << __FUNCTION__ << " -- ";
    QFont font = document()->defaultFont();
    font.setPointSizeF(13);
    document()->setDefaultFont(font);
}

void PlainTextEdit::zoomIn()
{
    qDebug().nospace() << __FILE__ << "(" << __LINE__ << ")" << __FUNCTION__ << " -- ";
    QFont font = document()->defaultFont();
    qreal fontSize = font.pointSizeF();
    fontSize = fontSize + 1;
    font.setPointSizeF(fontSize);
    document()->setDefaultFont(font);
}

void PlainTextEdit::zoomOut()
{
    qDebug().nospace() << __FILE__ << "(" << __LINE__ << ")" << __FUNCTION__ << " -- ";
    QFont font = document()->defaultFont();
    qreal fontSize = font.pointSizeF();
    fontSize = fontSize <= 6 ? fontSize : fontSize - 1;
    font.setPointSizeF(fontSize);
    document()->setDefaultFont(font);
}
QString PlainTextEdit::wordUnderCursor()
{
    int end = textCursor().position();
    int begin = end - 1;
    while (begin >= 0)
    {
        QChar ch = document()->characterAt(begin);
        if (!(ch.isLetterOrNumber() || ch == '.' || ch == '_'))
            break;
        begin--;
    }
    begin++;
    return document()->toPlainText().mid(begin, end - begin);
}

void PlainTextEdit::indentOrUnindent(bool doIndent)
{
    qDebug().nospace() << __FILE__ << "(" << __LINE__ << ")" << __FUNCTION__ << " -- ";
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    // Indent or unindent the selected lines
    if (cursor.hasSelection())
    {
        int            pos = cursor.position();
        int            anchor = cursor.anchor();
        int            start = qMin(anchor, pos);
        int            end = qMax(anchor, pos);
        QTextDocument* doc = document();
        QTextBlock     startBlock = doc->findBlock(start);
        QTextBlock     endBlock = doc->findBlock(end - 1).next();
        // Only one line partially selected.
        if (startBlock.next() == endBlock && (start > startBlock.position() || end < endBlock.position() - 1))
        {
            cursor.removeSelectedText();
        }
        else
        {
            for (QTextBlock block = startBlock; block != endBlock; block = block.next())
            {
                QString text = block.text();
                int     indentPosition = PlainTextEdit::lineIndentPosition(text);
                if (!doIndent && !indentPosition)
                {
                    indentPosition = PlainTextEdit::firstNonSpace(text);
                }
                int targetColumn = PlainTextEdit::indentedColumn(PlainTextEdit::columnAt(text, indentPosition), doIndent);
                cursor.setPosition(block.position() + indentPosition);
                cursor.insertText(PlainTextEdit::indentationString(0, targetColumn));
                cursor.setPosition(block.position());
                cursor.setPosition(block.position() + indentPosition, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
            }
            cursor.endEditBlock();
            return;
        }
    }
    // Indent or unindent at cursor position
    QTextBlock block = cursor.block();
    QString    text = block.text();
    int        indentPosition = cursor.positionInBlock();
    int        spaces = PlainTextEdit::spacesLeftFromPosition(text, indentPosition);
    int        startColumn = PlainTextEdit::columnAt(text, indentPosition - spaces);
    int        targetColumn = PlainTextEdit::indentedColumn(PlainTextEdit::columnAt(text, indentPosition), doIndent);
    cursor.setPosition(block.position() + indentPosition);
    cursor.setPosition(block.position() + indentPosition - spaces, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.insertText(PlainTextEdit::indentationString(startColumn, targetColumn));
    cursor.endEditBlock();
    setTextCursor(cursor);
}

LineNumberArea::LineNumberArea(QWidget* editor)
    : QWidget(editor)
{
    qDebug().nospace() << __FILE__ << "(" << __LINE__ << ")" << __FUNCTION__ << " -- ";
    this->mpEidtor = dynamic_cast<PlainTextEdit*>(editor);
//    setStyleSheet("QWidget{background-color: rgb(255, 255, 255);"
//                                    "border: 1px solid gray;}");
}

//QSize LineNumberArea::sizeHint() const
//{
//    qDebug().nospace() << __FILE__ << "(" << __LINE__ << ")" << __FUNCTION__ << " -- " << mpEidtor->lineNumberAreaWidth();
//    return QSize(mpEidtor->lineNumberAreaWidth(), 0);
//}

void LineNumberArea::paintEvent(QPaintEvent* event)
{
    qDebug().nospace() << __FILE__ << "(" << __LINE__ << ")" << __FUNCTION__ << " -- ";
    mpEidtor->lineNumberAreaPaintEvent(event);
    //    QWidget::paintEvent(event);
}

CompleterItem::CompleterItem(const QString& key, const QString& value, const QString& select)
    : mKey(key)
    , mValue(value)
    , mSelect(select)
{
    int ind = value.indexOf(select, 0);
    if (ind < 0)
    {
        mDescription = value;
    }
    else
    {
        mDescription = QString("<b>%1</b><i>%2</i>%3").arg(value.left(ind), select, value.right(value.size() - select.size() - ind)).replace("\n", "<br/>");
    }
}

CompleterItem::CompleterItem(const QString& key, const QString& value, const QString& select, const QString& description)
    : mKey(key)
    , mValue(value)
    , mSelect(select)
    , mDescription(description)
{
}

CompleterItem::CompleterItem(const QString& value, const QString& description)
    : mKey(value)
    , mValue(value)
    , mSelect(value)
    , mDescription(description)
{
}

TextHighlighter::TextHighlighter(QPlainTextEdit* pPlainTextEdit)
    : QSyntaxHighlighter(pPlainTextEdit->document())
{
    mpPlainTextEdit = pPlainTextEdit;
    initializeSettings();
}

void TextHighlighter::initializeSettings()
{
    QFont font;
    font.setFamily("Courier New");
    font.setPointSizeF(13);
    mpPlainTextEdit->document()->setDefaultFont(font);
    mpPlainTextEdit->setTabStopDistance((qreal)(4 * QFontMetrics(font).horizontalAdvance(QLatin1Char(' '))));
    mHighlightingRules.clear();

    HighlightingRule rule;

    mTextFormat.setForeground(QColor(0, 0, 0));                //文本
    mKeywordFormat.setForeground(QColor(0, 245, 255));         //关键字
    mTypeFormat.setForeground(QColor(255, 10, 10));            //类型
    mSingleLineCommentFormat.setForeground(QColor(0, 150, 0)); //单行注释
    mMultiLineCommentFormat.setForeground(QColor(0, 150, 0));  //多行注释
    mFunctionFormat.setForeground(QColor(0, 0, 255));
    mQuotationFormat.setForeground(QColor(0, 139, 0));
    mNumberFormat.setForeground(QColor(123, 104, 238)); //数字

    //为每个文本项设置 正则匹配
    rule.mPattern = QRegExp("[0-9][0-9]*([.][0-9]*)?([eE][+-]?[0-9]*)?");
    rule.mFormat = mNumberFormat;
    mHighlightingRules.append(rule);

    rule.mPattern = QRegExp("\\b[A-Za-z_][A-Za-z0-9_]*");
    rule.mFormat = mTextFormat;
    mHighlightingRules.append(rule);

    QStringList keywordPatterns = PlainTextEdit::getKeywords();
    foreach (const QString& pattern, keywordPatterns)
    {
        QString newPattern = QString("\\b%1\\b").arg(pattern);
        rule.mPattern = QRegExp(newPattern);
        rule.mFormat = mKeywordFormat;
        mHighlightingRules.append(rule);
    }

    QStringList typePatterns = PlainTextEdit::getTypes();
    foreach (const QString& pattern, typePatterns)
    {
        QString newPattern = QString("\\b%1\\b").arg(pattern);
        rule.mPattern = QRegExp(newPattern);
        rule.mFormat = mTypeFormat;
        mHighlightingRules.append(rule);
    }
}

void TextHighlighter::highlightBlock(const QString& text)
{
    //基本上每增加一个字母，这个函数执行一次
    qDebug().nospace() << __FILE__ << "(" << __LINE__ << ")" << __FUNCTION__ << " --  text = " << text;
    setCurrentBlockState(0);
    //    setFormat(0, text.length(), QColor(0, 0, 0));
    foreach (const HighlightingRule& rule, mHighlightingRules)
    {
        QRegExp expression(rule.mPattern);
        int     index = expression.indexIn(text);
        while (index >= 0)
        {
            int length = expression.matchedLength();
            setFormat(index, length, rule.mFormat);
            index = expression.indexIn(text, index + length);
        }
    }
    highlightMultiLine(text);
}

void TextHighlighter::highlightMultiLine(const QString& text)
{

    int index = 0, startIndex = 0;
    int blockState = previousBlockState();
    while (index < text.length())
    {
        switch (blockState)
        {
        /* if the block already has single line comment then don't check for multi line comment and quotes. */
        case 1:
            if (text[index] == '/' && index + 1 < text.length() && text[index + 1] == '/')
            {
                index++;
                blockState = 1; /* don't change the blockstate. */
            }
            break;
        case 2:
            if (text[index] == '*' && index + 1 < text.length() && text[index + 1] == '/')
            {
                index++;
                setFormat(startIndex, index - startIndex + 1, mMultiLineCommentFormat);
                blockState = 0;
            }
            break;
        case 3:
            if (text[index] == '\\')
            {
                index++;
            }
            else if (text[index] == '"')
            {
                setFormat(startIndex, index - startIndex + 1, mQuotationFormat);
                blockState = 0;
            }
            break;
        default:
            /* check if single line comment then set the blockstate to 1. */
            if (text[index] == '/' && index + 1 < text.length() && text[index + 1] == '/')
            {
                startIndex = index++;
                setFormat(startIndex, text.length(), mSingleLineCommentFormat);
                blockState = 1;
            }
            else if (text[index] == '/' && index + 1 < text.length() && text[index + 1] == '*')
            {
                startIndex = index++;
                blockState = 2;
            }
            else if (text[index] == '"')
            {
                startIndex = index;
                blockState = 3;
            }
        }
        index++;
    }
    switch (blockState)
    {
    case 2:
        setFormat(startIndex, text.length() - startIndex, mMultiLineCommentFormat);
        setCurrentBlockState(2);
        break;
    }
}
