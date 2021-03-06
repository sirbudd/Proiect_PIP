package com.arduino;

import com.arduino.ui.MainFormUI;

import javax.swing.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.File;

/**
 *
 * This is a Lock File
 * Folosim un Lock file ca sa fim siguri ca nu avem 2 controllere deschise in acelasi timp
 */

public class MainClass {

    private static File lock;

    public static boolean checkLock() {
        lock = new File(Constants.LOCK_FILE_NAME);

        return !lock.exists();
    }

    public static void createLock() throws Exception {
        lock.createNewFile();
        lock.deleteOnExit();
    }

    public static boolean freeLock() throws Exception {
        return lock.exists() && lock.delete();
    }

    public static void setNativeOSLookAndFeel() throws Exception {
        UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
    }

    /**
     * Initializare GUI
     * @param args
     */

    public static void main(final String[] args) {

        try {
            if (!checkLock()) {
                System.exit(0);
            }
            createLock();

            setNativeOSLookAndFeel();

            MainFormUI ui = new MainFormUI();
            ui.addWindowListener(new WindowAdapter() {
                @Override
                public void windowClosed(WindowEvent e) {
                    super.windowClosed(e);
                    try {
                        freeLock();
                    } catch (Exception ex) {
                        // If Freeing Lock Faces some Issues
                        System.exit(-1);
                    }
                    System.exit(0);
                }
            });
        } catch (Exception ex) {
            System.exit(-1);
        } finally {
        }
    }
}
