/*
 * User patch storage for the native builds.
 *
 * Patches are individual JSON files in the same flat shape the web app
 * exports (paramID -> value, plus "name"), so a patch saved here can be
 * imported in the browser and vice versa.
 *
 *   ~/Library/Application Support/Oh-a-synth/Patches/<name>.ohasynth.json
 *   ~/Library/Application Support/Oh-a-synth/favourites.txt
 *
 * Favourites are stored as ids: "F:NAME" for a factory preset, "U:NAME" for
 * a user patch, one per line.
 */
#pragma once

#include <juce_core/juce_core.h>

namespace oha {

struct PatchStore {
    static constexpr const char* extension = ".ohasynth.json";

    static juce::File root() {
        // On macOS userApplicationDataDirectory is ~/Library itself, so step
        // into Application Support to sit beside Oh-a-synth.settings rather
        // than cluttering the top of ~/Library.
        auto base = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
       #if JUCE_MAC
        base = base.getChildFile("Application Support");
       #endif
        auto d = base.getChildFile("Oh-a-synth");
        d.createDirectory();
        return d;
    }

    static juce::File patchDir() {
        auto d = root().getChildFile("Patches");
        d.createDirectory();
        return d;
    }

    static juce::String tidyName(const juce::String& n) {
        return n.trim().substring(0, 32);
    }

    static juce::Array<juce::File> files() {
        return patchDir().findChildFiles(juce::File::findFiles, false,
                                        juce::String("*") + extension);
    }

    static juce::String nameOf(const juce::File& f) {
        auto v = juce::JSON::parse(f);
        if (auto* o = v.getDynamicObject()) {
            const auto nm = o->getProperty("name").toString();
            if (nm.isNotEmpty()) return nm;
        }
        return f.getFileName().upToFirstOccurrenceOf(".", false, false);
    }

    // Display names of every stored patch, alphabetical.
    static juce::StringArray names() {
        juce::StringArray out;
        for (const auto& f : files()) out.addIfNotAlreadyThere(nameOf(f));
        out.sort(true);
        return out;
    }

    // The file holding a given patch, or where it would be written.
    static juce::File fileFor(const juce::String& name) {
        for (const auto& f : files())
            if (nameOf(f).equalsIgnoreCase(name)) return f;
        return patchDir().getChildFile(juce::File::createLegalFileName(tidyName(name)) + extension);
    }

    static bool save(const juce::String& name, const juce::var& patch) {
        return fileFor(name).replaceWithText(juce::JSON::toString(patch, false));
    }

    static juce::var load(const juce::String& name) {
        return juce::JSON::parse(fileFor(name));
    }

    static bool exists(const juce::String& name) {
        for (const auto& f : files())
            if (nameOf(f).equalsIgnoreCase(name)) return true;
        return false;
    }

    static bool remove(const juce::String& name) {
        auto f = fileFor(name);
        setFavourite("U:" + name, false);
        return f.existsAsFile() && f.deleteFile();
    }

    // ----- favourites -----
    static juce::File favouritesFile() { return root().getChildFile("favourites.txt"); }

    static juce::StringArray favourites() {
        juce::StringArray a;
        auto f = favouritesFile();
        if (f.existsAsFile()) a.addLines(f.loadFileAsString());
        a.removeEmptyStrings();
        return a;
    }

    static bool isFavourite(const juce::String& id) { return favourites().contains(id); }

    static void setFavourite(const juce::String& id, bool on) {
        auto a = favourites();
        if (on) a.addIfNotAlreadyThere(id);
        else    a.removeString(id);
        favouritesFile().replaceWithText(a.joinIntoString("\n"));
    }
};

} // namespace oha
