#include <QUdpSocket>
#include <QHostAddress>
// ... existing includes ...

void MapWindow::rebuildDreaming()
{
  saveDocument(); // Save first

  QUdpSocket socket;
  QByteArray data = "rebuild";
  socket.writeDatagram(data, QHostAddress("127.0.0.1"), 7777);
  
  // Maybe show a status message?
  // m_statusBarLabel->setText("Sent Rebuild Signal to GARA Engine..."); // Accessing private member might need getter or friend
}
