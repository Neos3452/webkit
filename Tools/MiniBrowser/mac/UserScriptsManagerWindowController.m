/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "UserScriptsManagerWindowController.h"

#if WK_API_ENABLED

#import "AppDelegate.h"
#import <WebKit/WKUserContentControllerPrivate.h>
#import <WebKit/WKUserScript.h>
#import <WebKit/WKUserScriptPrivate.h>
#import <WebKit/_WKUserContentWorld.h>

@interface ScriptCellInfo: NSObject

@property (copy) NSURL *url;
@property (copy) NSNumber *injectionTime;
@property (copy) NSArray *items;

@end

@implementation ScriptCellInfo

- (instancetype)initWithURL:(NSURL *)url
{
    self = [super init];
    if (self) {
        self.url = url;
        self.injectionTime = [NSNumber numberWithInt:WKUserScriptInjectionTimeAtDocumentEnd];
    }
    return self;
}

@end

@implementation UserScriptsManagerWindowController {
    _WKUserContentWorld *_userContentWorld;
    NSMutableDictionary<NSURL *, WKUserScript *> *_userScripts;
}

- (instancetype)init
{
    self = [self initWithWindowNibName:@"UserScriptsManagerWindowController"];
    if (self) {
        [NSValueTransformer setValueTransformer:
            [[WKUserScriptInjectionTimeToNameTransformer alloc] init]
            forName:@"WKUserScriptInjectionTimeToNameTransformer"];

        _userContentWorld = [[_WKUserContentWorld worldWithName:@"MiniBrowserContent"] retain];
        _userScripts = [[NSMutableDictionary<NSURL *, WKUserScript *> dictionary] retain];
    }
    return self;
}

- (void)loadInstalledUserScripts
{
    NSLog(@"Loading user scripts");
    NSArray* installedUserScripts = [[NSUserDefaults standardUserDefaults] arrayForKey:@"InstalledUserScripts"];
    if (installedUserScripts) {
        for (NSDictionary *scriptInfo in installedUserScripts) {
            NSError *readError = nil;
            NSString *path = [scriptInfo objectForKey:@"url"];
            [self loadScriptFromPath:[NSURL URLWithString:path] injectionTime:[[scriptInfo objectForKey:@"injectionTime"] intValue] error:&readError];
            if (readError) {
                NSLog(@"Script %@ coudn't not be loaded \"%@\"", path, [readError localizedDescription]);
            }
        }
    }
}

- (WKUserScript *)loadScriptFromPath:(NSURL *)path injectionTime:(WKUserScriptInjectionTime) injectionTime error:(NSError **) error
{
    NSString *scriptString = [[NSString alloc] initWithContentsOfURL:path encoding:NSUTF8StringEncoding error:error];
    if (!scriptString) {
        return nil;
    }

    WKUserScript *script = [[WKUserScript alloc] _initWithSource:scriptString injectionTime:injectionTime forMainFrameOnly:true legacyWhitelist:[NSArray array] legacyBlacklist:[NSArray array] associatedURL:path userContentWorld:_userContentWorld];

    [_userScripts setObject:script forKey:path];

    BrowserAppDelegate* appDelegate = (BrowserAppDelegate *)[[NSApplication sharedApplication] delegate];
    [appDelegate.userContentContoller addUserScript:script];
    return script;
}

//- (NSArray *)injectValues
//{
//    NSArray *arr = [NSArray arrayWithObjects:
//                    [NSNumber numberWithInt:WKUserScriptInjectionTimeAtDocumentStart],
//                    [NSNumber numberWithInt:WKUserScriptInjectionTimeAtDocumentEnd],
//                    nil];
//    return arr;
//}

- (void)windowDidLoad
{
    [super windowDidLoad];

//    [injectValues addObject:[NSNumber numberWithInt: WKUserScriptInjectionTimeAtDocumentStart]];
//    [injectValues addObject:[NSNumber numberWithInt: WKUserScriptInjectionTimeAtDocumentEnd]];
//    [injectValues addObject:@"WKUserScriptInjectionTimeAtDocumentStart"];
//    [injectValues addObject:@"WKUserScriptInjectionTimeAtDocumentEnd"];

    [_userScripts enumerateKeysAndObjectsUsingBlock:^(NSURL *url, WKUserScript *script, BOOL* stop) {
        ScriptCellInfo* cellInfo = [[[ScriptCellInfo alloc] initWithURL:url] autorelease];
        cellInfo.injectionTime = [NSNumber numberWithInt:script.injectionTime];
        cellInfo.items = [NSArray arrayWithObjects:
         [NSNumber numberWithInt:WKUserScriptInjectionTimeAtDocumentStart],
         [NSNumber numberWithInt:WKUserScriptInjectionTimeAtDocumentEnd],
         nil];
        [arrayController addObject:cellInfo];
    }];
}

- (void)dealloc
{
    [_userContentWorld release];
    [_userScripts release];

    [super dealloc];
}

- (IBAction)add:(id)sender
{
    NSOpenPanel *openPanel = [[NSOpenPanel openPanel] retain];
    openPanel.allowedFileTypes = @[ @"js" ];

    [openPanel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result)
    {
        if (result != NSFileHandlingPanelOKButton)
            return;

        ScriptCellInfo* cellInfo = [[[ScriptCellInfo alloc] initWithURL:[openPanel.URLs objectAtIndex:0]] autorelease];
        cellInfo.items = [NSArray arrayWithObjects:
                          [NSNumber numberWithInt:WKUserScriptInjectionTimeAtDocumentStart],
                          [NSNumber numberWithInt:WKUserScriptInjectionTimeAtDocumentEnd],
                          nil];
        [arrayController addObject:cellInfo];
    }];

}

- (IBAction)remove:(id)sender
{
    [arrayController removeObjectAtArrangedObjectIndex:[arrayController selectionIndex]];
}

- (IBAction)done:(id)sender
{
    BrowserAppDelegate* appDelegate = (BrowserAppDelegate *)[[NSApplication sharedApplication] delegate];
    [appDelegate.userContentContoller removeAllUserScripts];

    [_userScripts removeAllObjects];
    NSMutableArray* installedUserScripts = [NSMutableArray arrayWithCapacity:[arrayController.arrangedObjects count]];
    for (ScriptCellInfo* info in arrayController.arrangedObjects) {
        NSError *readError = nil;
        WKUserScript *script = [self loadScriptFromPath:info.url injectionTime:[info.injectionTime intValue] error:&readError];
        if (nil == readError && nil != script) {
            [_userScripts setObject:script forKey:info.url];
            [installedUserScripts addObject:@{@"url": [info.url absoluteString], @"injectionTime": info.injectionTime}];
        } else {
            NSAlert *alert = [NSAlert alertWithError:readError];
            [alert runModal];
        }
    }
    [[NSUserDefaults standardUserDefaults] setObject:installedUserScripts forKey:@"InstalledUserScripts"];
    [self close];
}
@end

@implementation WKUserScriptInjectionTimeToNameTransformer

+(Class)transformedValueClass {
    return [NSString class];
}

-(NSString *)transformedValue:(NSNumber *)value {
    WKUserScriptInjectionTime injectionTime = [value intValue];
    if (injectionTime == WKUserScriptInjectionTimeAtDocumentStart)
        return @"WKUserScriptInjectionTimeAtDocumentStart";
    else if (injectionTime == WKUserScriptInjectionTimeAtDocumentEnd)
        return @"WKUserScriptInjectionTimeAtDocumentEnd";

    NSLog(@"returning nil");
    return nil;
}

+(BOOL)allowsReverseTransformation {
    return YES;
}

-(NSNumber *)reverseTransformedValue:(NSString *)value {
    if ([@"WKUserScriptInjectionTimeAtDocumentStart" isEqualToString:value])
        return [NSNumber numberWithInt: WKUserScriptInjectionTimeAtDocumentStart];
    else if ([@"WKUserScriptInjectionTimeAtDocumentEnd" isEqualToString:value])
        return [NSNumber numberWithInt: WKUserScriptInjectionTimeAtDocumentEnd];

    return nil;
}

@end

#endif
