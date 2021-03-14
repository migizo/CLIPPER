/*
  ==============================================================================

    Knob.h
    Created: 12 Mar 2021 8:14:00am
    Author:  migizo

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "KnobResource.h"

class Knob : public juce::LookAndFeel_V4
{
public:
    Knob()
    {
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        cachedImage_knob = juce::ImageCache::getFromMemory (knob_png, knob_pngSize);

    }


    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider&) override
    {
        auto radius = (float) juce::jmin (width / 2, height / 2) - 4.0f;
        auto centreX = (float) x + (float) width  * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour (juce::Colours::black);
        // g.drawImage (cachedImage_knob, x, y, width, height, 0, 0, cachedImage_knob.getWidth(), cachedImage_knob.getHeight());
        auto imgW = cachedImage_knob.getWidth();
        auto imgH = cachedImage_knob.getHeight();
        auto pivotX = width/2-imgW/2;
        g.drawImageTransformed(cachedImage_knob, juce::AffineTransform::translation(pivotX, 0).rotated (angle, pivotX + imgW/2, imgH/2));
    }
    
    juce::Image cachedImage_knob;
    
    static const char* knob_png;
    static const int knob_pngSize;
};