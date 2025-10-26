#pragma once
#include "ZCamEventInterface.h"

class ZCamEvent : public ZCamEventInterface
{
public:
    ZCamEvent(const String &name, const String &cond, ZCamCallback cb)
        : _name(name), _cond(cond), _cb(cb) {}

    virtual ~ZCamEvent() {}

    void init() override {}
    void before(uint16_t[16]) override {}
    bool fire(uint16_t channels[16]) override
    {
        if (_cb)
            return _cb(_name, channels, _cond);
        return false;
    }
    void after(uint16_t[16], bool) override {}

    const String &name() const override { return _name; }
    const String &condition() const override { return _cond; }

    void attachCamera(ZCamCameraBaseInterface *cam) override { _cam = cam; }
    ZCamCameraBaseInterface *camera() const override { return _cam; }

protected:
    String _name;
    String _cond;
    ZCamCallback _cb = nullptr;
    ZCamCameraBaseInterface *_cam = nullptr;
};
