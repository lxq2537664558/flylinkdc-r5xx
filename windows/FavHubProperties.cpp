/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdafx.h"

#include "Resource.h"
#include "WinUtil.h"
#include "FavHubProperties.h"
#include "../client/UserCommand.h"

LRESULT FavHubProperties::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
#ifndef IRAINMAN_INCLUDE_HIDE_SHARE_MOD
	::EnableWindow(GetDlgItem(IDC_HIDE_SHARE), FALSE);
#endif
#ifndef IRAINMAN_ENABLE_STEALTH_MODE
	::EnableWindow(GetDlgItem(IDC_STEALTH), FALSE);
	GetDlgItem(IDC_STEALTH).ShowWindow(FALSE);
#endif
	// Translate dialog
	SetWindowText(CTSTRING(FAVORITE_HUB_PROPERTIES));
	SetDlgItemText(IDC_RAW_COMMANDS, CTSTRING(RAW_SET));
	SetDlgItemText(IDC_FH_HUB, CTSTRING(HUB));
	SetDlgItemText(IDC_FH_IDENT, CTSTRING(FAVORITE_HUB_IDENTITY));
	SetDlgItemText(IDC_FH_NAME, CTSTRING(HUB_NAME));
	SetDlgItemText(IDC_FH_ADDRESS, CTSTRING(HUB_ADDRESS));
	SetDlgItemText(IDC_FH_HUB_DESC, CTSTRING(DESCRIPTION));
	SetDlgItemText(IDC_FH_NICK, CTSTRING(NICK));
	SetDlgItemText(IDC_FH_PASSWORD, CTSTRING(PASSWORD));
	SetDlgItemText(IDC_FH_USER_DESC, CTSTRING(DESCRIPTION));
	SetDlgItemText(IDC_FH_EMAIL, CTSTRING(EMAIL));
	SetDlgItemText(IDC_FH_AWAY, CTSTRING(AWAY));
	SetDlgItemText(IDC_DEFAULT, CTSTRING(DEFAULT));
	SetDlgItemText(IDC_ACTIVE, CTSTRING(SETTINGS_DIRECT));
	SetDlgItemText(IDC_PASSIVE, CTSTRING(SETTINGS_FIREWALL_PASSIVE));
#ifdef IRAINMAN_INCLUDE_HIDE_SHARE_MOD
	SetDlgItemText(IDC_HIDE_SHARE, CTSTRING(HIDE_SHARE));
#endif
	SetDlgItemText(IDC_SHOW_JOINS, CTSTRING(SHOW_JOINS));
	SetDlgItemText(IDC_EXCL_CHECKS, CTSTRING(EXCL_CHECKS));
	SetDlgItemText(IDC_EXCLUSIVE_HUB, CTSTRING(EXCLUSIVE_HUB));
#ifdef IRAINMAN_ENABLE_STEALTH_MODE
	SetDlgItemText(IDC_STEALTH, CTSTRING(STEALTH_MODE));
	STEALTH_MODE, // "Emulate DC++ in this hub"
#endif
	SetDlgItemText(IDC_RAW1, Text::toT(SETTING(RAW1_TEXT)).c_str());
	SetDlgItemText(IDC_RAW2, Text::toT(SETTING(RAW2_TEXT)).c_str());
	SetDlgItemText(IDC_RAW3, Text::toT(SETTING(RAW3_TEXT)).c_str());
	SetDlgItemText(IDC_RAW4, Text::toT(SETTING(RAW4_TEXT)).c_str());
	SetDlgItemText(IDC_RAW5, Text::toT(SETTING(RAW5_TEXT)).c_str());
	SetDlgItemText(IDC_OPCHAT, CTSTRING(OPCHAT));
	SetDlgItemText(IDC_CONN_BORDER, CTSTRING(CONNECTION));
	SetDlgItemText(IDC_FAV_SEARCH_INTERVAL, CTSTRING(MINIMUM_SEARCH_INTERVAL));
	SetDlgItemText(IDC_S, CTSTRING(S));
	SetDlgItemText(IDC_CLIENT_ID, CTSTRING(CLIENT_ID)); // !SMT!-S
	SetDlgItemText(IDC_ENCODING, CTSTRING(FAVORITE_HUB_ENCODING));
	SetDlgItemText(IDCANCEL, CTSTRING(CANCEL));
	SetDlgItemText(IDOK, CTSTRING(OK));
	SetDlgItemText(IDC_FAVGROUP, CTSTRING(GROUP));
	SetDlgItemText(IDC_ENCODINGTEXT, CTSTRING(FAVORITE_HUB_ENCODINGTEXT));
	// Fill in values
	SetDlgItemText(IDC_HUBNAME, Text::toT(entry->getName()).c_str());
	SetDlgItemText(IDC_HUBDESCR, Text::toT(entry->getDescription()).c_str());
	SetDlgItemText(IDC_HUBADDR, Text::toT(entry->getServer()).c_str());
	SetDlgItemText(IDC_HUBNICK, Text::toT(entry->getNick(false)).c_str());
	SetDlgItemText(IDC_HUBPASS, Text::toT(entry->getPassword()).c_str());
	SetDlgItemText(IDC_HUBUSERDESCR, Text::toT(entry->getUserDescription()).c_str());
	SetDlgItemText(IDC_HUBAWAY, Text::toT(entry->getAwayMsg()).c_str());
	SetDlgItemText(IDC_HUBEMAIL, Text::toT(entry->getEmail()).c_str());
#ifdef IRAINMAN_ENABLE_STEALTH_MODE
	CheckDlgButton(IDC_STEALTH, entry->getStealth() ? BST_CHECKED : BST_UNCHECKED);
#endif
#ifdef IRAINMAN_INCLUDE_HIDE_SHARE_MOD
	CheckDlgButton(IDC_HIDE_SHARE, entry->getHideShare() ? BST_CHECKED : BST_UNCHECKED);
#endif
	CheckDlgButton(IDC_SHOW_JOINS, entry->getShowJoins() ? BST_CHECKED : BST_UNCHECKED); // Show joins
	CheckDlgButton(IDC_EXCL_CHECKS, entry->getExclChecks() ? BST_CHECKED : BST_UNCHECKED); // Excl. from client checking
	CheckDlgButton(IDC_EXCLUSIVE_HUB, entry->getExclusiveHub() ? BST_CHECKED : BST_UNCHECKED); // Exclusive hub, send H:1/0/0 or similar
	SetDlgItemText(IDC_RAW_ONE, Text::toT(entry->getRawOne()).c_str());
	SetDlgItemText(IDC_RAW_TWO, Text::toT(entry->getRawTwo()).c_str());
	SetDlgItemText(IDC_RAW_THREE, Text::toT(entry->getRawThree()).c_str());
	SetDlgItemText(IDC_RAW_FOUR, Text::toT(entry->getRawFour()).c_str());
	SetDlgItemText(IDC_RAW_FIVE, Text::toT(entry->getRawFive()).c_str());
	SetDlgItemText(IDC_SERVER, Text::toT(entry->getIP()).c_str());
	SetDlgItemText(IDC_OPCHAT_STR, Text::toT(entry->getOpChat()).c_str());
	SetDlgItemText(IDC_FAV_SEARCH_INTERVAL_BOX, Util::toStringW(entry->getSearchInterval()).c_str());
	
	SetDlgItemText(IDC_WIZARD_NICK_RND, CTSTRING(WIZARD_NICK_RND)); // Rand Nick button
	SetDlgItemText(IDC_WIZARD_NICK_RND2, CTSTRING(DEFAULT));        // Default Nick button
//	CString login;
//	login.SetString(Text::toT( entry->getNick(false) ).c_str());
//	SetDlgItemText(IDC_HUBNICK, login);

	// [+] IRainman mimicry function. Thanks SMT!
	IdCombo.Attach(GetDlgItem(IDC_CLIENT_ID_BOX));
	const bool l_isAdc = Util::isAdcHub(entry->getServer());
	for (const FavoriteManager::mimicrytag* i = &FavoriteManager::g_MimicryTags[0]; i->tag; ++i)
	{
		IdCombo.AddString(Text::toT(FavoriteManager::createClientId(i->tag, i->version, l_isAdc)).c_str());
	}
	
	
	if (!entry->getClientName().empty())
		SetDlgItemText(IDC_CLIENT_ID_BOX, Text::toT(FavoriteManager::createClientId(entry->getClientName(), entry->getClientVersion(), l_isAdc)).c_str());
		
	CheckDlgButton(IDC_CLIENT_ID, entry->getOverrideId() ? BST_CHECKED : BST_UNCHECKED);
	BOOL x;
	OnChangeId(BN_CLICKED, IDC_CLIENT, 0, x);
	// [~] IRainman mimicry function
	
	CComboBox combo;
	combo.Attach(GetDlgItem(IDC_FAVGROUP_BOX));
	combo.AddString(_T("---"));
	combo.SetCurSel(0);
	
	{
		FavoriteManager::LockInstanceHubs lockedInstanceHubs;
		const FavHubGroups& favHubGroups = lockedInstanceHubs.getFavHubGroups();
		for (auto i = favHubGroups.cbegin(); i != favHubGroups.cend(); ++i)
		{
			const string& name = i->first;
			int pos = combo.AddString(Text::toT(name).c_str());
			
			if (name == entry->getGroup())
				combo.SetCurSel(pos);
		}
	}
	
	combo.Detach();
	
	// [!] IRainman fix.
	combo.Attach(GetDlgItem(IDC_ENCODING));
	if (Util::isAdcHub(entry->getServer()))
	{
		// select UTF-8 for ADC hubs
		combo.AddString(Text::toT(Text::g_utf8).c_str());
		
		combo.SetCurSel(0);
		combo.EnableWindow(false);
	}
	else
	{
		// TODO: add more encoding into wxWidgets version, this is enough now
		// FIXME: following names are Windows only!
		combo.AddString(_T("System default"));
		combo.AddString(Text::toT(Text::g_code1251).c_str());
		combo.AddString(Text::toT(Text::g_utf8).c_str());
		combo.AddString(Text::toT(Text::g_code1252).c_str());
		combo.AddString(_T(""));
		combo.AddString(_T("Czech_Czech Republic.1250"));
		
		if (entry->getEncoding().empty())
			combo.SetCurSel(0);
		else
			combo.SetWindowText(Text::toT(entry->getEncoding()).c_str());
	}
	// [~] IRainman fix.
	
	combo.Detach();
	
	CUpDownCtrl updown;
	updown.Attach(GetDlgItem(IDC_FAV_SEARCH_INTERVAL_SPIN));
	updown.SetRange(1, 500); //[+]NightOrion
	updown.Detach();
	
	if (entry->getMode() == 0)
		CheckRadioButton(IDC_ACTIVE, IDC_DEFAULT, IDC_DEFAULT);
	else if (entry->getMode() == 1)
		CheckRadioButton(IDC_ACTIVE, IDC_DEFAULT, IDC_ACTIVE);
	else if (entry->getMode() == 2)
		CheckRadioButton(IDC_ACTIVE, IDC_DEFAULT, IDC_PASSIVE);
		
	CEdit tmp;
	tmp.Attach(GetDlgItem(IDC_HUBNAME));
	tmp.SetFocus();
	tmp.SetSel(0, -1);
	tmp.Detach();
	tmp.Attach(GetDlgItem(IDC_HUBNICK));
	tmp.LimitText(35);
	tmp.Detach();
	tmp.Attach(GetDlgItem(IDC_HUBUSERDESCR));
	tmp.LimitText(50);
	tmp.Detach();
	tmp.Attach(GetDlgItem(IDC_HUBPASS));
	tmp.SetPasswordChar('*');
	tmp.Detach();
	CenterWindow(GetParent());
	
	return FALSE;
}

LRESULT FavHubProperties::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (wID == IDOK)
	{
		tstring buf;
		GET_TEXT(IDC_HUBADDR, buf);
		if (buf.empty())
		{
			MessageBox(CTSTRING(INCOMPLETE_FAV_HUB), _T(""), MB_ICONWARNING | MB_OK);
			return 0;
		}
		const string& url = Text::fromT(buf);
		entry->setServer(Util::formatDchubUrl(url));
		GET_TEXT(IDC_HUBNAME, buf);
		entry->setName(Text::fromT(buf));
		GET_TEXT(IDC_HUBDESCR, buf);
		entry->setDescription(Text::fromT(buf));
		GET_TEXT(IDC_HUBNICK, buf);
		entry->setNick(Text::fromT(buf));
		GET_TEXT(IDC_HUBPASS, buf);
		entry->setPassword(Text::fromT(buf));
		GET_TEXT(IDC_HUBUSERDESCR, buf);
		entry->setUserDescription(Text::fromT(buf));
		GET_TEXT(IDC_HUBAWAY, buf);
		entry->setAwayMsg(Text::fromT(buf));
		GET_TEXT(IDC_HUBEMAIL, buf);
		entry->setEmail(Text::fromT(buf));
#ifdef IRAINMAN_ENABLE_STEALTH_MODE
		entry->setStealth(IsDlgButtonChecked(IDC_STEALTH) == 1);
#endif
#ifdef IRAINMAN_INCLUDE_HIDE_SHARE_MOD
		entry->setHideShare(IsDlgButtonChecked(IDC_HIDE_SHARE) == 1);
#endif
		entry->setShowJoins(IsDlgButtonChecked(IDC_SHOW_JOINS) == 1); // Show joins
		entry->setExclChecks(IsDlgButtonChecked(IDC_EXCL_CHECKS) == 1); // Excl. from client checking
		entry->setExclusiveHub(IsDlgButtonChecked(IDC_EXCLUSIVE_HUB) == 1); // Exclusive hub, send H:1/0/0 or similar
		GET_TEXT(IDC_RAW_ONE, buf);
		entry->setRawOne(Text::fromT(buf));
		GET_TEXT(IDC_RAW_TWO, buf);
		entry->setRawTwo(Text::fromT(buf));
		GET_TEXT(IDC_RAW_THREE, buf);
		entry->setRawThree(Text::fromT(buf));
		GET_TEXT(IDC_RAW_FOUR, buf);
		entry->setRawFour(Text::fromT(buf));
		GET_TEXT(IDC_RAW_FIVE, buf);
		entry->setRawFive(Text::fromT(buf));
		GET_TEXT(IDC_SERVER, buf);
		entry->setIP(Text::fromT(buf));
		GET_TEXT(IDC_OPCHAT_STR, buf);
		entry->setOpChat(Text::fromT(buf));
		GET_TEXT(IDC_FAV_SEARCH_INTERVAL_BOX, buf);
		entry->setSearchInterval(Util::toUInt32(Text::fromT(buf)));
		
		CComboBox combo;
		combo.Attach(GetDlgItem(IDC_FAVGROUP_BOX));
		
		if (combo.GetCurSel() == 0)
		{
			entry->setGroup(Util::emptyString);
		}
		else
		{
			tstring text;
			WinUtil::GetWindowText(text, combo);
			entry->setGroup(Text::fromT(text));
		}
		combo.Detach();
		
		// [+] IRainman mimicry function. Thanks SMT!
		GET_TEXT(IDC_CLIENT_ID_BOX, buf);
		string l_clientName, l_clientVersion;
		FavoriteManager::splitClientId(Text::fromT(buf), l_clientName, l_clientVersion);
		entry->setClientName(l_clientName);
		entry->setClientVersion(l_clientVersion);
		entry->setOverrideId(IsDlgButtonChecked(IDC_CLIENT_ID) == BST_CHECKED);
		// [~] IRainman mimicry function
		
		
		int ct = -1;
		if (IsDlgButtonChecked(IDC_DEFAULT))
			ct = 0;
		else if (IsDlgButtonChecked(IDC_ACTIVE))
			ct = 1;
		else if (IsDlgButtonChecked(IDC_PASSIVE))
			ct = 2;
			
		entry->setMode(ct);
		
		// [!] IRainman fix.
		if (Util::isAdcHub(entry->getServer()))
		{
			entry->setEncoding(Text::g_utf8);
		}
		else
		{
			GET_TEXT(IDC_ENCODING, buf);
			if (_tcschr(buf.data(), _T('.')) == NULL && _tcscmp(buf.data(), Text::toT(Text::g_utf8).c_str()) != 0 && _tcscmp(buf.data(), _T("System default")) != 0) // TODO translate
			{
				MessageBox(CTSTRING(INVALID_ENCODING), _T(""), MB_ICONWARNING | MB_OK);
				return 0;
			}
			entry->setEncoding(Text::fromT(buf));
		}
		// [~] IRainman fix.
		
		FavoriteManager::getInstance()->save();
	}
	EndDialog(wID);
	return 0;
}

LRESULT FavHubProperties::OnTextChanged(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
{
	wstring buf;
	
	GET_TEXT(wID, buf);
	wstring old = buf;
	
	// TODO: move to Text and cleanup.
	if (!buf.empty())
	{
		// Strip '$', '|' and ' ' from text
		wchar_t *b = &buf[0], *f = &buf[0], c;
		while ((c = *b++) != 0)
		{
			if (c != '$' && c != '|' && (wID == IDC_HUBUSERDESCR || c != ' ') && ((wID != IDC_HUBNICK && wID != IDC_HUBUSERDESCR && wID != IDC_HUBEMAIL) || (c != '<' && c != '>')))
				*f++ = c;
		}
		*f = '\0';
	}
	if (old != buf)
	{
		// Something changed; update window text without changing cursor pos
		CEdit tmp;
		tmp.Attach(hWndCtl);
		int start, end;
		tmp.GetSel(start, end);
		tmp.SetWindowText(buf.data());
		if (start > 0) start--;
		if (end > 0) end--;
		tmp.SetSel(start, end);
		tmp.Detach();
	}
	
	return TRUE;
}

// !SMT!-S
LRESULT FavHubProperties::OnChangeId(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	::EnableWindow(GetDlgItem(IDC_CLIENT_ID_BOX), (IsDlgButtonChecked(IDC_CLIENT_ID) == BST_CHECKED));
	return 0;
}

LRESULT FavHubProperties::onRandomNick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// [!] IRainman fix: Please stop copy-past!
	//// Create RND nick
	//const size_t iNickLength = 16;
	//const char samples[] = "0123456789";
	//const char samples2[] = "_-";
	//const char samples3[] = "User,Monk,Boys,Girl,True,Head,Hell,Hulk,Hawk,Fire,";
	//
	//std::vector<char> name(iNickLength);
	//rand();
	//size_t val = (size_t)( (double)rand() / (RAND_MAX + 1) * (sizeof(samples3) / 5 ) ) * 5;
	//name[0] = samples3[val];
	//name[1] = samples3[val+1];
	//name[2] = samples3[val+2];
	//name[3] = samples3[val+3];
	//size_t val2 = (size_t)( (double)rand() / (RAND_MAX + 1) * ( sizeof(samples2) - 1 ) );
	//name[4] = samples2[val2];
	//
	//for (size_t i = 5; i<iNickLength-1; i++)
	//{
	//  size_t value = (size_t)( (double)rand() / (RAND_MAX + 1) * ( sizeof(samples) - 1 ) );
	//  name[i] = samples[value];
	//}
	//name[iNickLength-1] = 0;
	//
	//CString login;
	//login.SetString(Text::toT(&name[0]).c_str());
	
	SetDlgItemText(IDC_HUBNICK, Text::toT(Util::getRandomNick()).c_str());
	
	return 0;
}

LRESULT FavHubProperties::onDefaultNick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDlgItemText(IDC_HUBNICK, Text::toT(SETTING(NICK)).c_str());
	return 0;
}

/**
 * @file
 * $Id: FavHubProperties.cpp,v 1.18 2006/08/06 17:51:33 bigmuscle Exp $
 */