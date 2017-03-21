#include "chrome/browser/certificate_viewer.h"

#import "chrome/browser/ui/certificate_viewer_mac.h"

void ShowCertificateViewer(content::WebContents* web_contents,
                           gfx::NativeWindow parent,
                           net::X509Certificate* cert) {
  SSLCertificateViewerMac* viewer =  ([[SSLCertificateViewerMac alloc]
                                      initWithCertificate:cert
                                      forWebContents:web_contents]);
  [viewer showCertificateSheet:parent];
  [viewer releaseSheetWindow];
}
