#pragma once

#include <QString>

namespace DesktopAuthBridge
{
inline QString MethodOnLoginSuccess()
{
    return QStringLiteral("Desktop.OnLoginSuccess");
}

inline QString MethodRouteChanged()
{
    return QStringLiteral("Desktop.RouteChanged");
}

inline QString BridgeInjectScript()
{
    return QStringLiteral(R"JS(
      (function() {
        if (window.__DESKTOP_QT__ && window.__DESKTOP_QT__.__ready) {
          return;
        }
        function notifyRouteChanged() {
          try {
            if (window.CallBridge && typeof window.CallBridge.invoke === 'function') {
              window.CallBridge.invoke('Desktop.RouteChanged', { href: String(location.href || '') });
            }
          } catch (e) {
          }
        }
        try {
          var _pushState = history.pushState;
          history.pushState = function() {
            var ret = _pushState.apply(this, arguments);
            notifyRouteChanged();
            return ret;
          };
          var _replaceState = history.replaceState;
          history.replaceState = function() {
            var ret = _replaceState.apply(this, arguments);
            notifyRouteChanged();
            return ret;
          };
        } catch (e) {
        }
        try {
          window.addEventListener('hashchange', notifyRouteChanged, true);
          window.addEventListener('popstate', notifyRouteChanged, true);
          window.addEventListener('DOMContentLoaded', notifyRouteChanged, true);
          window.addEventListener('load', notifyRouteChanged, true);
        } catch (e) {
        }

        window.__DESKTOP_QT__ = {
          __ready: true,
          onLoginSuccess: function(payload) {
            if (window.CallBridge && typeof window.CallBridge.invoke === 'function') {
              window.CallBridge.invoke('Desktop.OnLoginSuccess', payload || {});
              return true;
            }
            return false;
          }
        };
        notifyRouteChanged();
      })();
    )JS");
}
}
